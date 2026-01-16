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

#include <Server/Server.h>
#include <Server/Simulation.h>
#include <Server/Client.h>
#include <Shared/Crypto.h>
#include <Shared/pb.h>
#include <emscripten.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/time.h>

// We need to access server_tick, but it's static in Server.c
// For worker mode, we'll need to make it non-static or duplicate the logic
// For now, we'll declare it here - this will require making it non-static in Server.c
extern void server_tick_worker(struct rr_server *this);

// Forward declaration for client cleanup
extern void rr_server_client_free(struct rr_server_client *this);

static struct rr_server *g_server = NULL;

void rr_server_worker_init(void)
{
    if (g_server != NULL)
        return;
    
    g_server = calloc(1, sizeof(struct rr_server));
    rr_server_init(g_server);
    // In worker mode, api_ws_ready should be set to true
    g_server->api_ws_ready = 1;
}

void rr_server_worker_tick(void)
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
    
    // Call the server tick function
    server_tick_worker(g_server);
}

// Handle new client connection (equivalent to LWS_CALLBACK_ESTABLISHED)
// out_buffer must be at least 1024 bytes
uint32_t rr_server_worker_handle_connect(uint8_t *out_buffer)
{
    if (g_server == NULL || !g_server->api_ws_ready || out_buffer == NULL)
    {
        return 0;
    }
    
    // In single-player mode, always use client slot 0
    // Clean up any existing client first
    uint64_t client_idx = 0;
    if (rr_bitset_get(g_server->clients_in_use, client_idx))
    {
        // Clean up existing client
        rr_bitset_unset(g_server->clients_in_use, client_idx);
        g_server->clients[client_idx].in_use = 0;
        rr_server_client_free(&g_server->clients[client_idx]);
    }
    
    // Create the client
    rr_bitset_set(g_server->clients_in_use, client_idx);
    rr_server_client_init(&g_server->clients[client_idx]);
    g_server->clients[client_idx].server = g_server;
    g_server->clients[client_idx].in_use = 1;
    
    printf("<rr_server::worker_connect::verification=%llu>\n", 
           (unsigned long long)g_server->clients[client_idx].requested_verification);
    
    // Send encryption keys (like LWS_CALLBACK_ESTABLISHED does)
    extern uint8_t *outgoing_message;
    struct proto_bug encryption_key_encoder;
    proto_bug_init(&encryption_key_encoder, outgoing_message);
    proto_bug_write_uint64(&encryption_key_encoder,
                           g_server->clients[client_idx].requested_verification,
                           "verification");
    proto_bug_write_uint32(&encryption_key_encoder, rr_get_rand(),
                           "useless bytes");
    proto_bug_write_uint64(&encryption_key_encoder,
                           g_server->clients[client_idx].clientbound_encryption_key,
                           "c encryption key");
    proto_bug_write_uint64(&encryption_key_encoder,
                           g_server->clients[client_idx].serverbound_encryption_key,
                           "s encryption key");
    // Skip encryption in single-player mode
    // rr_encrypt(outgoing_message, 1024, 21094093777837637ull);
    // rr_encrypt(outgoing_message, 8, 1);
    // rr_encrypt(outgoing_message, 1024, 59731158950470853ull);
    // rr_encrypt(outgoing_message, 1024, 64709235936361169ull);
    // rr_encrypt(outgoing_message, 1024, 59013169977270713ull);
    
    // Copy to output buffer (no encryption in single-player mode)
    memcpy(out_buffer, outgoing_message, encryption_key_encoder.current - encryption_key_encoder.start);
    return encryption_key_encoder.current - encryption_key_encoder.start;
}

void rr_server_worker_handle_message(uint8_t *data, uint32_t size)
{
    if (g_server == NULL || data == NULL || size == 0)
        return;
    
    // In single-player mode, always use client slot 0
    if (!rr_bitset_get(g_server->clients_in_use, 0))
    {
        return; // No client connected
    }
    
    struct rr_server_client *client = &g_server->clients[0];
    
    // Skip decryption in single-player mode
    // No decryption in single-player mode
    // rr_decrypt(data, size, client->serverbound_encryption_key);
    // client->serverbound_encryption_key =
    //     rr_get_hash(rr_get_hash(client->serverbound_encryption_key));
    
    // Process the message through the server's message handling
    // This needs to call the equivalent of handle_lws_event with LWS_CALLBACK_RECEIVE
    // For now, we'll use a simplified version that processes the first packet
    if (!client->received_first_packet)
    {
        client->received_first_packet = 1;
        
        struct proto_bug encoder;
        proto_bug_init(&encoder, data);
        proto_bug_set_bound(&encoder, data + size);
        
        proto_bug_read_uint64(&encoder, "useless bytes");
        uint64_t received_verification = proto_bug_read_uint64(&encoder, "verification");
        printf("<rr_server::worker_received_verification::expected=%llu::got=%llu>\n",
               (unsigned long long)client->requested_verification, 
               (unsigned long long)received_verification);
        if (received_verification != client->requested_verification)
        {
            printf("Invalid verification: expected %llu, got %llu\n", 
                   (unsigned long long)client->requested_verification, (unsigned long long)received_verification);
            return;
        }
        
        memset(&client->rivet_account, 0, sizeof(struct rr_rivet_account));
        proto_bug_read_string(&encoder, client->rivet_account.token, 300, "rivet token");
        proto_bug_read_string(&encoder, client->rivet_account.uuid, 100, "rivet uuid");
        
        if (proto_bug_read_varuint(&encoder, "dev_flag") == 49453864343)
            client->dev = 1;
        
        printf("<rr_server::socket_verified::%s>\n", client->rivet_account.uuid);
        
        // In worker mode, we need to load account from local storage
        #ifdef RR_WORKER_MODE
        extern int rr_worker_storage_load_account(char *uuid, struct rr_server_client *client);
        int account_loaded = rr_worker_storage_load_account(client->rivet_account.uuid, client);
        if (!account_loaded)
        {
            // Account doesn't exist yet - initialize with default values
            printf("<rr_server::account_not_found::creating_new>\n");
            client->experience = 0;
            memset(client->inventory, 0, sizeof(client->inventory));
            memset(client->craft_fails, 0, sizeof(client->craft_fails));
            // Give new accounts 5 common basic petals (like the client expects)
            // rr_petal_id_basic = 1, rr_rarity_id_common = 0
            client->inventory[1][0] = 5;
            // Save the new account
            extern void rr_worker_storage_save_account(char *uuid, struct rr_server_client *client);
            rr_worker_storage_save_account(client->rivet_account.uuid, client);
        }
        client->verified = 1;
        printf("<rr_server::account_loaded::verified=1>\n");
        // Send account data to client (like the normal server does)
        extern void rr_server_client_write_account(struct rr_server_client *client);
        rr_server_client_write_account(client);
        printf("<rr_server::account_message_queued>\n");
        #endif
        return;
    }
    
    if (!client->verified)
    {
        printf("<rr_server::worker_message_rejected::not_verified>\n");
        return;
    }
    
    // Process normal game messages
    // Reinitialize encoder after decryption
    struct proto_bug encoder;
    proto_bug_init(&encoder, data);
    proto_bug_set_bound(&encoder, data + size);
    uint8_t header = proto_bug_read_uint8(&encoder, "header");
    
    printf("<rr_server::worker_handle_message::header=0x%02x::size=%u::verified=%d>\n", header, size, client->verified);
    
    // Call the extracted message handling function from Server.c
    extern void server_handle_client_message(struct rr_server *server, struct rr_server_client *client, struct proto_bug *encoder, uint8_t header);
    server_handle_client_message(g_server, client, &encoder, header);
}

uint32_t rr_server_worker_get_message(uint8_t **out_data)
{
    if (g_server == NULL)
    {
        *out_data = NULL;
        return 0;
    }
    
    // Check for outgoing messages from client slot 0 (single-player mode)
    uint64_t client_idx = 0;
    if (!rr_bitset_get(g_server->clients_in_use, client_idx))
    {
        *out_data = NULL;
        return 0;
    }
    
    struct rr_server_client *client = &g_server->clients[client_idx];
    
    // Get the first message from the queue
    if (client->message_root == NULL)
    {
        *out_data = NULL;
        return 0;
    }
    
    struct rr_server_client_message *message = client->message_root;
    client->message_root = message->next;
    if (client->message_root == NULL)
        client->message_at = NULL;
    
    // Return the message data (packet is already at LWS_PRE offset, but LWS_PRE is 0 in worker mode)
    *out_data = message->packet;
    uint32_t len = message->len;
    
    // Free the message structure (but not the packet - caller will free it)
    free(message);
    
    return len;
}

