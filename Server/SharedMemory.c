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

#include <Server/SharedMemory.h>
#include <Server/Server.h>
#include <Server/Client.h>
#include <Server/Simulation.h>
#include <Shared/Crypto.h>
#include <Shared/pb.h>
#include <emscripten.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

#ifdef RR_WORKER_MODE

extern void server_tick_worker(struct rr_server *this);
extern void rr_server_client_free(struct rr_server_client *this);
extern void server_handle_client_message(struct rr_server *server, struct rr_server_client *client, struct proto_bug *encoder, uint8_t header);
extern uint8_t *outgoing_message;

static struct rr_server *g_server = NULL;
static struct rr_shared_memory *g_shared_mem = NULL;

void rr_shared_memory_init(struct rr_shared_memory *mem)
{
    if (mem == NULL)
        return;
    memset(mem, 0, sizeof(struct rr_shared_memory));
    g_shared_mem = mem;
}

void rr_server_shared_init(void)
{
    if (g_server != NULL)
        return;
    
    g_server = calloc(1, sizeof(struct rr_server));
    rr_server_init(g_server);
    g_server->api_ws_ready = 1;
    
    // Initialize client slot 0 for single-player mode
    extern void rr_server_client_init(struct rr_server_client *client);
    rr_bitset_set(g_server->clients_in_use, 0);
    rr_server_client_init(&g_server->clients[0]);
    g_server->clients[0].server = g_server;
    g_server->clients[0].in_use = 1;
    
    // Send initial packet with verification keys (like LWS_CALLBACK_ESTABLISHED does)
    extern uint8_t *outgoing_message;
    struct proto_bug encryption_key_encoder;
    proto_bug_init(&encryption_key_encoder, outgoing_message);
    proto_bug_write_uint64(&encryption_key_encoder,
                           g_server->clients[0].requested_verification,
                           "verification");
    proto_bug_write_uint32(&encryption_key_encoder, rr_get_rand(),
                           "useless bytes");
    proto_bug_write_uint64(&encryption_key_encoder,
                           g_server->clients[0].clientbound_encryption_key,
                           "c encryption key");
    proto_bug_write_uint64(&encryption_key_encoder,
                           g_server->clients[0].serverbound_encryption_key,
                           "s encryption key");
    rr_server_shared_send_message(encryption_key_encoder.start,
                                  encryption_key_encoder.current - encryption_key_encoder.start);
    printf("<rr_server::shared_init::sent_initial_packet::verification=%llu>\n",
           (unsigned long long)g_server->clients[0].requested_verification);
    
    if (g_shared_mem)
        g_shared_mem->server_initialized = 1;
}

void rr_server_shared_tick(void)
{
    if (g_server == NULL)
        return;
    
    // Rate limit server ticks to ~25 ticks per second (40ms per tick)
    static struct timeval last_tick_time = {0, 0};
    struct timeval current_time;
    gettimeofday(&current_time, NULL);
    
    if (last_tick_time.tv_sec == 0 && last_tick_time.tv_usec == 0)
    {
        // First tick, initialize
        last_tick_time = current_time;
    }
    else
    {
        // Calculate elapsed time in microseconds
        int64_t elapsed = (current_time.tv_sec - last_tick_time.tv_sec) * 1000000 +
                         (current_time.tv_usec - last_tick_time.tv_usec);
        
        // Only tick if at least 40ms (40000 microseconds) have passed
        // This limits to ~25 ticks per second
        if (elapsed < 40000)
        {
            // Too soon, skip this tick
            return;
        }
        
        last_tick_time = current_time;
    }
    
    // Process incoming messages from client
    if (g_shared_mem)
    {
        uint8_t message_buffer[65536];
        uint32_t size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
        while (size > 0)
        {
            // Handle the message
            if (!rr_bitset_get(g_server->clients_in_use, 0))
            {
                // No client connected yet
                size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
                continue;
            }
            
            struct rr_server_client *client = &g_server->clients[0];
            
            // Handle first packet (connection)
            if (!client->received_first_packet)
            {
                client->received_first_packet = 1;
                
                struct proto_bug encoder;
                proto_bug_init(&encoder, message_buffer);
                proto_bug_set_bound(&encoder, message_buffer + size);
                
                proto_bug_read_uint64(&encoder, "useless bytes");
                uint64_t received_verification = proto_bug_read_uint64(&encoder, "verification");
                
                if (received_verification != client->requested_verification)
                {
                    printf("Invalid verification: expected %llu, got %llu\n", 
                           (unsigned long long)client->requested_verification, 
                           (unsigned long long)received_verification);
                    size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
                    continue;
                }
                
                memset(&client->rivet_account, 0, sizeof(struct rr_rivet_account));
                proto_bug_read_string(&encoder, client->rivet_account.token, 300, "rivet token");
                proto_bug_read_string(&encoder, client->rivet_account.uuid, 100, "rivet uuid");
                
                if (proto_bug_read_varuint(&encoder, "dev_flag") == 49453864343)
                    client->dev = 1;
                
                printf("<rr_server::socket_verified::%s>\n", client->rivet_account.uuid);
                
                // Load account from storage
                extern int rr_worker_storage_load_account(char *uuid, struct rr_server_client *client);
                int account_loaded = rr_worker_storage_load_account(client->rivet_account.uuid, client);
                if (!account_loaded)
                {
                    printf("<rr_server::account_not_found::creating_new>\n");
                    client->experience = 0;
                    memset(client->inventory, 0, sizeof(client->inventory));
                    memset(client->craft_fails, 0, sizeof(client->craft_fails));
                    client->inventory[1][0] = 5; // 5 common basic petals
                    extern void rr_worker_storage_save_account(char *uuid, struct rr_server_client *client);
                    rr_worker_storage_save_account(client->rivet_account.uuid, client);
                }
                client->verified = 1;
                printf("<rr_server::account_loaded::verified=1>\n");
                
                // Send encryption keys response (no encryption in single-player)
                struct proto_bug response_encoder;
                proto_bug_init(&response_encoder, outgoing_message);
                proto_bug_write_uint64(&response_encoder, client->requested_verification, "verification");
                proto_bug_write_uint32(&response_encoder, rr_get_rand(), "useless bytes");
                proto_bug_write_uint64(&response_encoder, client->clientbound_encryption_key, "c encryption key");
                proto_bug_write_uint64(&response_encoder, client->serverbound_encryption_key, "s encryption key");
                rr_server_shared_send_message(response_encoder.start, response_encoder.current - response_encoder.start);
                
                // Send account data
                extern void rr_server_client_write_account(struct rr_server_client *client);
                rr_server_client_write_account(client);
                
                // Send initial update message to enable buttons (even without being in a squad)
                // Create a temporary squad entry for the client so the update message works
                extern uint8_t rr_client_create_squad(struct rr_server *server, struct rr_server_client *client);
                extern uint8_t rr_client_join_squad(struct rr_server *server, struct rr_server_client *client, uint8_t pos);
                uint8_t squad = rr_client_create_squad(g_server, client);
                if (squad != 0xFF) // 0xFF is RR_ERROR_CODE_INVALID_SQUAD
                {
                    rr_client_join_squad(g_server, client, squad);
                    printf("<rr_server::auto_joined_squad::squad=%u>\n", squad);
                    // Don't set playing=1 yet - wait for "Enter Game" button
                    extern void rr_server_client_broadcast_update(struct rr_server_client *client);
                    rr_server_client_broadcast_update(client);
                    printf("<rr_server::sent_initial_update>\n");
                }
                
                if (g_shared_mem)
                    g_shared_mem->connection_established = 1;
                
                size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
                continue;
            }
            
            if (!client->verified)
            {
                size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
                continue;
            }
            
            // Process normal game messages
            struct proto_bug encoder;
            proto_bug_init(&encoder, message_buffer);
            proto_bug_set_bound(&encoder, message_buffer + size);
            // Validate encoder bounds
            if (encoder.end < encoder.start || encoder.end - encoder.start < size)
            {
                printf("<rr_server::encoder_bounds_invalid::start=%p::end=%p::size=%u::expected_size=%llu::skipping_message>\n",
                       (void*)encoder.start, (void*)encoder.end, 
                       (unsigned)size,
                       (unsigned long long)(encoder.end - encoder.start));
                size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
                continue;
            }
            printf("<rr_server::shared_memory::message_received::size=%u::encoder_start=%p::encoder_end=%p::encoder_current=%p>\n",
                   (unsigned)size,
                   (void*)encoder.start,
                   (void*)encoder.end,
                   (void*)encoder.current);
            uint8_t header = proto_bug_read_uint8(&encoder, "header");
            printf("<rr_server::shared_memory::header_read::header=0x%02x::encoder_current=%p::bytes_read=%llu>\n",
                   (unsigned)header,
                   (void*)encoder.current,
                   (unsigned long long)(encoder.current - encoder.start));
            
            server_handle_client_message(g_server, client, &encoder, header);
            
            size = rr_server_shared_get_client_message(message_buffer, sizeof(message_buffer));
        }
    }
    
    // Run server tick
    server_tick_worker(g_server);
}

uint32_t rr_server_shared_get_client_message(uint8_t *buffer, uint32_t max_size)
{
    if (g_shared_mem == NULL || buffer == NULL || max_size == 0)
        return 0;
    
    uint32_t read_pos = g_shared_mem->client_to_server_read_pos;
    uint32_t write_pos = g_shared_mem->client_to_server_write_pos;
    
    if (read_pos == write_pos)
        return 0; // No messages
    
    // Read message size (first 4 bytes)
    // Handle wrap-around: if size bytes cross buffer boundary, read in chunks
    uint32_t message_size = 0;
    uint32_t size_read_pos = read_pos;
    if (size_read_pos + 4 <= sizeof(g_shared_mem->client_to_server_buffer))
    {
        // Size fits in one chunk
        memcpy(&message_size, &g_shared_mem->client_to_server_buffer[size_read_pos], 4);
    }
    else
    {
        // Size wraps around - read in two chunks
        uint32_t first_chunk = sizeof(g_shared_mem->client_to_server_buffer) - size_read_pos;
        uint32_t second_chunk = 4 - first_chunk;
        memcpy((uint8_t*)&message_size, &g_shared_mem->client_to_server_buffer[size_read_pos], first_chunk);
        memcpy((uint8_t*)&message_size + first_chunk, &g_shared_mem->client_to_server_buffer[0], second_chunk);
    }
    read_pos = (read_pos + 4) % (sizeof(g_shared_mem->client_to_server_buffer));
    
    // Validate message size
    if (message_size > max_size || message_size == 0 || message_size > sizeof(g_shared_mem->client_to_server_buffer))
    {
        printf("<rr_server::shared_get::invalid_message_size::size=%u::max=%u::read_pos=%u::write_pos=%u>\n", 
               (unsigned)message_size, (unsigned)max_size, (unsigned)read_pos, (unsigned)write_pos);
        // Skip invalid message - advance read_pos past the size bytes only
        // Don't try to skip message_size bytes as it might be corrupted
        g_shared_mem->client_to_server_read_pos = read_pos;
        return 0;
    }
    
    // Read message data
    uint32_t bytes_to_read = message_size;
    uint32_t bytes_read = 0;
    while (bytes_read < bytes_to_read)
    {
        uint32_t chunk_size = bytes_to_read - bytes_read;
        uint32_t available = sizeof(g_shared_mem->client_to_server_buffer) - read_pos;
        if (chunk_size > available)
            chunk_size = available;
        
        memcpy(buffer + bytes_read, &g_shared_mem->client_to_server_buffer[read_pos], chunk_size);
        read_pos = (read_pos + chunk_size) % (sizeof(g_shared_mem->client_to_server_buffer));
        bytes_read += chunk_size;
    }
    
    g_shared_mem->client_to_server_read_pos = read_pos;
    return message_size;
}

void rr_server_shared_send_message(uint8_t *data, uint32_t size)
{
    if (g_shared_mem == NULL)
    {
        printf("<rr_server::shared_send::g_shared_mem_is_null>\n");
        return;
    }
    if (data == NULL || size == 0)
    {
        printf("<rr_server::shared_send::invalid_params::data=%p::size=%u>\n", (void*)data, (unsigned)size);
        return;
    }
    
    uint32_t write_pos = g_shared_mem->server_to_client_write_pos;
    uint32_t read_pos = g_shared_mem->server_to_client_read_pos;
    
    // Check if there's enough space (leave some headroom)
    uint32_t available = (read_pos > write_pos) ? 
        (read_pos - write_pos - 1) : 
        (sizeof(g_shared_mem->server_to_client_buffer) - write_pos + read_pos - 1);
    
    if (size + 4 > available)
    {
        printf("<rr_server::shared_memory_full::dropping_message::size=%u::available=%u::write_pos=%u::read_pos=%u>\n", 
               (unsigned)size, (unsigned)available, (unsigned)write_pos, (unsigned)read_pos);
        return; // Buffer full, drop message
    }
    
    // Write message size
    memcpy(&g_shared_mem->server_to_client_buffer[write_pos], &size, 4);
    write_pos = (write_pos + 4) % (sizeof(g_shared_mem->server_to_client_buffer));
    
    // Write message data
    uint32_t bytes_written = 0;
    while (bytes_written < size)
    {
        uint32_t chunk_size = size - bytes_written;
        uint32_t available_space = sizeof(g_shared_mem->server_to_client_buffer) - write_pos;
        if (chunk_size > available_space)
            chunk_size = available_space;
        
        memcpy(&g_shared_mem->server_to_client_buffer[write_pos], data + bytes_written, chunk_size);
        write_pos = (write_pos + chunk_size) % (sizeof(g_shared_mem->server_to_client_buffer));
        bytes_written += chunk_size;
    }
    
    g_shared_mem->server_to_client_write_pos = write_pos;
    printf("<rr_server::shared_send::message_written::size=%u::write_pos=%u::read_pos=%u>\n", 
           (unsigned)size, (unsigned)write_pos, (unsigned)read_pos);
}

uint32_t rr_client_shared_get_server_message(uint8_t *buffer, uint32_t max_size)
{
    if (g_shared_mem == NULL)
    {
        static int warned = 0;
        if (!warned)
        {
            printf("<rr_client::shared_get::g_shared_mem_is_null>\n");
            warned = 1;
        }
        return 0;
    }
    if (buffer == NULL || max_size == 0)
        return 0;
    
    uint32_t read_pos = g_shared_mem->server_to_client_read_pos;
    uint32_t write_pos = g_shared_mem->server_to_client_write_pos;
    
    if (read_pos == write_pos)
        return 0; // No messages
    
    // Read message size (first 4 bytes)
    // Handle wrap-around: if size bytes cross buffer boundary, read in chunks
    uint32_t message_size = 0;
    uint32_t size_read_pos = read_pos;
    if (size_read_pos + 4 <= sizeof(g_shared_mem->server_to_client_buffer))
    {
        // Size fits in one chunk
        memcpy(&message_size, &g_shared_mem->server_to_client_buffer[size_read_pos], 4);
    }
    else
    {
        // Size wraps around - read in two chunks
        uint32_t first_chunk = sizeof(g_shared_mem->server_to_client_buffer) - size_read_pos;
        uint32_t second_chunk = 4 - first_chunk;
        memcpy((uint8_t*)&message_size, &g_shared_mem->server_to_client_buffer[size_read_pos], first_chunk);
        memcpy((uint8_t*)&message_size + first_chunk, &g_shared_mem->server_to_client_buffer[0], second_chunk);
    }
    read_pos = (read_pos + 4) % (sizeof(g_shared_mem->server_to_client_buffer));
    
    // Validate message size
    if (message_size > max_size || message_size == 0 || message_size > sizeof(g_shared_mem->server_to_client_buffer))
    {
        printf("<rr_client::shared_get::invalid_message_size::size=%u::max=%u::read_pos=%u::write_pos=%u>\n", 
               (unsigned)message_size, (unsigned)max_size, (unsigned)read_pos, (unsigned)write_pos);
        // Skip invalid message - advance read_pos past the size bytes only
        // Don't try to skip message_size bytes as it might be corrupted
        g_shared_mem->server_to_client_read_pos = read_pos;
        return 0;
    }
    
    // Read message data
    uint32_t bytes_to_read = message_size;
    uint32_t bytes_read = 0;
    while (bytes_read < bytes_to_read)
    {
        uint32_t chunk_size = bytes_to_read - bytes_read;
        uint32_t available = sizeof(g_shared_mem->server_to_client_buffer) - read_pos;
        if (chunk_size > available)
            chunk_size = available;
        
        memcpy(buffer + bytes_read, &g_shared_mem->server_to_client_buffer[read_pos], chunk_size);
        read_pos = (read_pos + chunk_size) % (sizeof(g_shared_mem->server_to_client_buffer));
        bytes_read += chunk_size;
    }
    
    g_shared_mem->server_to_client_read_pos = read_pos;
    printf("<rr_client::shared_get::message_read::size=%u::read_pos=%u::write_pos=%u>\n", 
           (unsigned)message_size, (unsigned)read_pos, (unsigned)write_pos);
    return message_size;
}

void rr_client_shared_send_message(uint8_t *data, uint32_t size)
{
    if (g_shared_mem == NULL || data == NULL || size == 0)
        return;
    
    uint32_t write_pos = g_shared_mem->client_to_server_write_pos;
    uint32_t read_pos = g_shared_mem->client_to_server_read_pos;
    
    // Check if there's enough space (leave some headroom)
    uint32_t available = (read_pos > write_pos) ? 
        (read_pos - write_pos - 1) : 
        (sizeof(g_shared_mem->client_to_server_buffer) - write_pos + read_pos - 1);
    
    if (size + 4 > available)
    {
        printf("<rr_client::shared_memory_full::dropping_message>\n");
        return; // Buffer full, drop message
    }
    
    // Write message size (handle wrap-around if needed)
    if (write_pos + 4 <= sizeof(g_shared_mem->client_to_server_buffer))
    {
        // Size fits in one chunk
        memcpy(&g_shared_mem->client_to_server_buffer[write_pos], &size, 4);
    }
    else
    {
        // Size wraps around - write in two chunks
        uint32_t first_chunk = sizeof(g_shared_mem->client_to_server_buffer) - write_pos;
        uint32_t second_chunk = 4 - first_chunk;
        memcpy(&g_shared_mem->client_to_server_buffer[write_pos], (uint8_t*)&size, first_chunk);
        memcpy(&g_shared_mem->client_to_server_buffer[0], (uint8_t*)&size + first_chunk, second_chunk);
    }
    write_pos = (write_pos + 4) % (sizeof(g_shared_mem->client_to_server_buffer));
    
    // Write message data
    uint32_t bytes_written = 0;
    while (bytes_written < size)
    {
        uint32_t chunk_size = size - bytes_written;
        uint32_t available_space = sizeof(g_shared_mem->client_to_server_buffer) - write_pos;
        if (chunk_size > available_space)
            chunk_size = available_space;
        
        memcpy(&g_shared_mem->client_to_server_buffer[write_pos], data + bytes_written, chunk_size);
        write_pos = (write_pos + chunk_size) % (sizeof(g_shared_mem->client_to_server_buffer));
        bytes_written += chunk_size;
    }
    
    g_shared_mem->client_to_server_write_pos = write_pos;
}

#endif // RR_WORKER_MODE

