// Copyright (C) 2024  Paul Johnson
//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

let serverInitialized = false;
let clientConnections = new Map();
let messageQueue = [];

// Import the Emscripten-generated WASM loader first
// It will declare Module
importScripts('rrolf-server-worker.js');

// Now configure Module's onRuntimeInitialized callback
// This must be done immediately after importScripts but before Module initializes
// Emscripten modules initialize asynchronously, so we should have time
if (typeof Module !== 'undefined') {
    // Store the original callback if it exists
    const originalCallback = Module.onRuntimeInitialized;
    
    Module.onRuntimeInitialized = function() {
        console.log('Server WASM module loaded');
        Module._rr_server_worker_init();
        serverInitialized = true;
        
        // Process any queued messages
        while (messageQueue.length > 0) {
            const msg = messageQueue.shift();
            handleMessage(msg);
        }
        
        // Start server tick loop
        tick();
        
        // Call original callback if it existed
        if (originalCallback) {
            originalCallback();
        }
    };
} else {
    console.error('Module not found after importing rrolf-server-worker.js');
}

function tick() {
    if (serverInitialized) {
        try {
            Module._rr_server_worker_tick();
            
            // Process ALL queued messages, not just one
            // This ensures messages are sent immediately without delay
            let messagesProcessed = 0;
            const maxMessagesPerTick = 100; // Prevent infinite loop
            
            while (messagesProcessed < maxMessagesPerTick) {
                // Allocate space for the pointer (4 bytes for 32-bit pointer)
                const messagePtrPtr = Module._malloc(4);
                const size = Module._rr_server_worker_get_message(messagePtrPtr);
                
                if (size > 0) {
                    // Read the pointer from the allocated memory (32-bit pointer)
                    const dataPtr = Module.HEAPU32[messagePtrPtr / 4];
                    
                    if (dataPtr) {
                        // Copy message data
                        const messageData = new Uint8Array(size);
                        messageData.set(new Uint8Array(Module.HEAPU8.buffer, dataPtr, size));
                        
                        // Send to all connected clients
                        clientConnections.forEach((conn, clientId) => {
                            if (conn.connected) {
                                const dataCopy = new Uint8Array(messageData); // Make a copy for transfer
                                self.postMessage({
                                    type: 'message',
                                    clientId: clientId,
                                    data: dataCopy.buffer
                                }, [dataCopy.buffer]);
                            }
                        });
                        
                        // Free the message buffer
                        Module._free(dataPtr);
                        messagesProcessed++;
                    }
                } else {
                    // No more messages
                    Module._free(messagePtrPtr);
                    break;
                }
                
                Module._free(messagePtrPtr);
            }
        } catch (error) {
            console.error('[Worker] Error in tick:', error);
        }
    }
    // Run at ~25 ticks per second (40ms per tick) to match server tickrate
    setTimeout(tick, 40);
}

function handleMessage(event) {
    if (!serverInitialized) {
        messageQueue.push(event);
        return;
    }
    
    const data = event.data;
    if (data.type === 'connect') {
        // New client connection - call C function to establish connection
        const clientId = data.clientId;
        
        // Allocate buffer for encryption keys response
        const responsePtr = Module._malloc(1024);
        const responseSize = Module._rr_server_worker_handle_connect(responsePtr);
        
        if (responseSize > 0) {
            // Copy response data to a new ArrayBuffer
            const responseData = new Uint8Array(responseSize);
            responseData.set(new Uint8Array(Module.HEAPU8.buffer, responsePtr, responseSize));
            
            clientConnections.set(clientId, {
                id: clientId,
                connected: true
            });
            
            // Send encryption keys to client
            self.postMessage({
                type: 'message',
                clientId: clientId,
                data: responseData.buffer
            }, [responseData.buffer]);
        }
        
        Module._free(responsePtr);
    } else if (data.type === 'message') {
        // Forward message to server WASM
        const clientId = data.clientId;
        const messageData = data.data;
        
        if (messageData instanceof ArrayBuffer) {
            try {
                const uint8Data = new Uint8Array(messageData);
                const ptr = Module._malloc(uint8Data.length);
                if (ptr === 0) {
                    console.error('[Worker] Failed to allocate memory for message');
                    return;
                }
                Module.HEAPU8.set(uint8Data, ptr);
                Module._rr_server_worker_handle_message(ptr, uint8Data.length);
                Module._free(ptr);
            } catch (error) {
                console.error('[Worker] Error handling message:', error);
            }
        }
    } else if (data.type === 'disconnect') {
        // Client disconnected
        const clientId = data.clientId;
        clientConnections.delete(clientId);
    }
}

// Handle messages from main thread
self.onmessage = function(event) {
    handleMessage(event);
};

// Handle storage for API replacement using indexedDB (workers don't have localStorage)
let storageDB = null;
let storageCache = {}; // In-memory cache for synchronous access

function initStorage() {
    return new Promise((resolve, reject) => {
        const request = indexedDB.open('rrolf_storage', 1);
        request.onsuccess = function(event) {
            storageDB = event.target.result;
            // Load all accounts into cache
            if (storageDB.objectStoreNames.contains('accounts')) {
                const transaction = storageDB.transaction(['accounts'], 'readonly');
                const store = transaction.objectStore('accounts');
                const getAllRequest = store.getAll();
                getAllRequest.onsuccess = function() {
                    getAllRequest.result.forEach(account => {
                        if (account && account.uuid) {
                            storageCache[`rrolf_account_${account.uuid}`] = account;
                        }
                    });
                    resolve(storageDB);
                };
                getAllRequest.onerror = function() {
                    resolve(storageDB);
                };
            } else {
                resolve(storageDB);
            }
        };
        request.onerror = function(event) {
            reject(event);
        };
        request.onupgradeneeded = function(event) {
            const db = event.target.result;
            if (!db.objectStoreNames.contains('accounts')) {
                db.createObjectStore('accounts');
            }
            if (!db.objectStoreNames.contains('settings')) {
                db.createObjectStore('settings');
            }
        };
    });
}

function saveToStorage(uuid, data) {
    const key = `rrolf_account_${uuid}`;
    // Update cache immediately for synchronous access
    storageCache[key] = data;
    
    // Save to indexedDB asynchronously
    if (!storageDB) {
        initStorage().then(() => saveToStorage(uuid, data));
        return;
    }
    const transaction = storageDB.transaction(['accounts'], 'readwrite');
    const store = transaction.objectStore('accounts');
    store.put(data, key);
}

function loadFromStorage(uuid) {
    const key = `rrolf_account_${uuid}`;
    // Return from cache for synchronous access
    const result = storageCache[key] || null;
    console.log(`[WorkerStorage] loadFromStorage: uuid=${uuid}, key=${key}, found=${!!result}`);
    if (result) {
        console.log(`[WorkerStorage] Account data:`, result);
    }
    return result;
}

// Expose to Module for C code
if (typeof Module !== 'undefined') {
    Module.saveToStorage = saveToStorage;
    Module.loadFromStorage = loadFromStorage;
}

// Initialize storage when module loads
if (typeof Module !== 'undefined' && Module.onRuntimeInitialized) {
    const originalCallback = Module.onRuntimeInitialized;
    Module.onRuntimeInitialized = function() {
        initStorage().then(() => {
            if (originalCallback) originalCallback();
        });
    };
} else {
    initStorage();
}

