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

#pragma once

#include <stdint.h>

#ifdef RR_WORKER_MODE

// Shared memory structure for client-server communication
// This is allocated in JavaScript and passed to both client and server
#define SHARED_MEMORY_SIZE (1024 * 1024 * 4) // 4MB shared memory

struct rr_shared_memory
{
    // Message queues
    uint32_t client_to_server_write_pos;  // Where client writes
    uint32_t client_to_server_read_pos;   // Where server reads
    uint32_t server_to_client_write_pos; // Where server writes
    uint32_t server_to_client_read_pos;  // Where client reads
    
    // Message buffers (circular buffers)
    uint8_t client_to_server_buffer[SHARED_MEMORY_SIZE / 2];
    uint8_t server_to_client_buffer[SHARED_MEMORY_SIZE / 2];
    
    // Control flags
    uint32_t server_initialized;
    uint32_t client_connected;
    uint32_t connection_established;
};

// Initialize shared memory (called from JavaScript)
void rr_shared_memory_init(struct rr_shared_memory *mem);

// Server functions
void rr_server_shared_init(void);
void rr_server_shared_tick(void);
uint32_t rr_server_shared_get_client_message(uint8_t *buffer, uint32_t max_size);
void rr_server_shared_send_message(uint8_t *data, uint32_t size);

// Client functions (to be called from client code)
uint32_t rr_client_shared_get_server_message(uint8_t *buffer, uint32_t max_size);
void rr_client_shared_send_message(uint8_t *data, uint32_t size);

#endif // RR_WORKER_MODE

