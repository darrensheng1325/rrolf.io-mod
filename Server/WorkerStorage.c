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

#include <Server/WorkerStorage.h>
#include <emscripten.h>
#include <string.h>
#include <stdlib.h>

#ifdef RR_WORKER_MODE

void rr_worker_storage_save_account(char *uuid, struct rr_server_client *client)
{
    double experience = client->experience;
    uint32_t *inventory_ptr = (uint32_t*)client->inventory;
    uint32_t *craft_fails_ptr = (uint32_t*)client->craft_fails;
    
    EM_ASM(
        {
            const uuid = UTF8ToString($0);
            const key = 'rrolf_account_' + uuid;
            
            // Get account data from client structure
            const experience = HEAPF64[$1 / 8];
            const inventoryPtr = $2;
            const craftFailsPtr = $3;
            
            // Read inventory data
            const inventory = {};
            for (let id = 1; id < 255; id++) {
                for (let rarity = 0; rarity < 8; rarity++) {
                    const count = HEAPU32[(inventoryPtr + (id * 8 + rarity) * 4) / 4];
                    if (count > 0) {
                        if (!inventory[id]) inventory[id] = {};
                        inventory[id][rarity] = count;
                    }
                }
            }
            
            // Read craft fails data
            const craftFails = {};
            for (let id = 1; id < 255; id++) {
                for (let rarity = 0; rarity < 8; rarity++) {
                    const count = HEAPU32[(craftFailsPtr + (id * 8 + rarity) * 4) / 4];
                    if (count > 0) {
                        if (!craftFails[id]) craftFails[id] = {};
                        craftFails[id][rarity] = count;
                    }
                }
            }
            
            const accountData = {};
            accountData.uuid = uuid;
            accountData.experience = experience;
            accountData.inventory = inventory;
            accountData.craftFails = craftFails;
            
            // Save to localStorage (synchronous, simple)
            try {
                window.localStorage.setItem(key, JSON.stringify(accountData));
                console.log('[WorkerStorage] Saved account to localStorage for uuid:', uuid);
            } catch (e) {
                console.error('[WorkerStorage] Error saving to localStorage:', e);
                // Fallback: try IndexedDB if localStorage fails
                if (typeof indexedDB !== 'undefined') {
                    try {
                        const request = indexedDB.open('rrolf_storage', 1);
                        request.onsuccess = function(event) {
                            const db = event.target.result;
                            if (!db.objectStoreNames.contains('accounts')) {
                                const upgradeRequest = indexedDB.open('rrolf_storage', 2);
                                upgradeRequest.onupgradeneeded = function(e) {
                                    e.target.result.createObjectStore('accounts');
                                };
                            } else {
                                const transaction = db.transaction(['accounts'], 'readwrite');
                                const store = transaction.objectStore('accounts');
                                store.put(accountData, key);
                            }
                        };
                        request.onupgradeneeded = function(event) {
                            const db = event.target.result;
                            if (!db.objectStoreNames.contains('accounts')) {
                                db.createObjectStore('accounts');
                            }
                        };
                    } catch (e2) {
                        console.error('[WorkerStorage] Error saving to IndexedDB:', e2);
                    }
                }
            }
        },
        uuid, &experience, inventory_ptr, craft_fails_ptr);
}

int rr_worker_storage_load_account(char *uuid, struct rr_server_client *client)
{
    int result = 0;
    
    double *experience_ptr = &client->experience;
    uint32_t *inventory_ptr = (uint32_t*)client->inventory;
    uint32_t *craft_fails_ptr = (uint32_t*)client->craft_fails;
    
    result = EM_ASM_INT(
        {
            const uuid = UTF8ToString($0);
            const key = 'rrolf_account_' + uuid;
            
            // Try to load from localStorage first (synchronous, simple)
            let accountData = null;
            try {
                const stored = window.localStorage.getItem(key);
                if (stored) {
                    accountData = JSON.parse(stored);
                    console.log('[WorkerStorage] Loaded account from localStorage for uuid:', uuid);
                }
            } catch (e) {
                console.error('[WorkerStorage] Error loading from localStorage:', e);
            }
            
            // If not in localStorage, try IndexedDB (async, but we'll use a simple approach)
            if (!accountData && typeof indexedDB !== 'undefined') {
                // For now, return 0 if not in localStorage
                // IndexedDB access would require async handling which is complex
                console.log('[WorkerStorage] Account not found in localStorage for uuid:', uuid);
            }
            
            if (!accountData) {
                console.log('[WorkerStorage] No account data found for uuid:', uuid);
                return 0;
            }
            
            try {
                
                // Write experience
                HEAPF64[$1 / 8] = accountData.experience || 0;
                
                // Write inventory - first clear it
                const inventoryPtr = $2;
                // Clear existing inventory in C memory before loading
                for (let id = 0; id < 255; id++) {
                    for (let rarity = 0; rarity < 8; rarity++) {
                        HEAPU32[(inventoryPtr + (id * 8 + rarity) * 4) / 4] = 0;
                    }
                }
                if (accountData.inventory) {
                    for (const id in accountData.inventory) {
                        const idNum = parseInt(id);
                        for (const rarity in accountData.inventory[id]) {
                            const rarityNum = parseInt(rarity);
                            const count = accountData.inventory[id][rarity];
                            HEAPU32[(inventoryPtr + (idNum * 8 + rarityNum) * 4) / 4] = count;
                        }
                    }
                }
                
                // Write craft fails - first clear it
                const craftFailsPtr = $3;
                // Clear existing craft fails in C memory before loading
                for (let id = 0; id < 255; id++) {
                    for (let rarity = 0; rarity < 8; rarity++) {
                        HEAPU32[(craftFailsPtr + (id * 8 + rarity) * 4) / 4] = 0;
                    }
                }
                if (accountData.craftFails) {
                    for (const id in accountData.craftFails) {
                        const idNum = parseInt(id);
                        for (const rarity in accountData.craftFails[id]) {
                            const rarityNum = parseInt(rarity);
                            const count = accountData.craftFails[id][rarity];
                            HEAPU32[(craftFailsPtr + (idNum * 8 + rarityNum) * 4) / 4] = count;
                        }
                    }
                }
                
                return 1;
            } catch (e) {
                console.error('Error loading account data:', e);
                return 0;
            }
        },
        uuid, experience_ptr, inventory_ptr, craft_fails_ptr);
    
    return result;
}

void rr_worker_storage_set_server_alias(char *alias)
{
    EM_ASM(
        {
            const alias = UTF8ToString($0);
            // Use indexedDB instead of localStorage
            const request = indexedDB.open('rrolf_storage', 1);
            request.onsuccess = function(event) {
                const db = event.target.result;
                const transaction = db.transaction(['settings'], 'readwrite');
                const store = transaction.objectStore('settings');
                store.put(alias, 'rrolf_server_alias');
            };
            request.onupgradeneeded = function(event) {
                const db = event.target.result;
                if (!db.objectStoreNames.contains('settings')) {
                    db.createObjectStore('settings');
                }
            };
        },
        alias);
}

void rr_worker_storage_get_server_alias(char *alias, size_t size)
{
    EM_ASM(
        {
            // Use default for now (settings can be loaded asynchronously)
            const result = 'localhost';
            stringToUTF8(result, $0, $1);
        },
        alias, size);
}

#endif // RR_WORKER_MODE

