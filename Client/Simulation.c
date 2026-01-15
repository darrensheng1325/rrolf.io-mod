// Copyright (C) 2024  Paul Johnson

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as
// published by the Free Software Foundation, either version 3 of the
// License, or (at your option) any later version.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Affero General Public License for more details.

// You should have received a copy of the GNU Affero General Public License
// along with this program.  If not, see <https://www.gnu.org/licenses/>.

#include <Client/Simulation.h>

#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>

#include <Client/Game.h>
#include <Client/System/DeletionAnimation.h>
#include <Client/System/Interpolation.h>
#include <Shared/Bitset.h>
#include <Shared/pb.h>
#ifdef SINGLE_PLAYER_BUILD
#include <Shared/StaticData.h>
#endif

#ifndef SINGLE_PLAYER_BUILD
// In non-single-player mode, client provides its own version
void rr_simulation_init(struct rr_simulation *this)
{
    memset(this, 0, sizeof *this);
}
#endif
// In single-player mode, server's version is used (defined in Server/Simulation.c)
// But the client needs arena initialized for rendering, so we provide a helper function
#ifdef SINGLE_PLAYER_BUILD
void rr_simulation_init_client_arena(struct rr_simulation *this)
{
    // Initialize arena for client simulation (needed for rendering)
    // This is called after rr_simulation_init to set up the arena
    // The arena must be entity 1 because tick_maze expects it at that ID
    if (this->arena_count == 0)
    {
        // Check if entity 1 already exists
        EntityIdx id = 1;
        if (rr_simulation_has_entity(this, id))
        {
            // Entity 1 exists but doesn't have arena - add it
            if (!rr_simulation_has_arena(this, id))
            {
                struct rr_component_arena *arena = rr_simulation_add_arena(this, id);
                arena->biome = rr_biome_id_hell_creek; // Always use hell creek
                // Initialize the maze pointer so tick_maze doesn't fail
                extern struct rr_maze_declaration RR_MAZES[];
                arena->maze = &RR_MAZES[rr_biome_id_hell_creek];
            }
        }
        else
        {
            // Entity 1 doesn't exist - create it with arena
            this->entity_tracker[id] = 1;
            struct rr_component_arena *arena = rr_simulation_add_arena(this, id);
            arena->biome = rr_biome_id_hell_creek; // Always use hell creek
            // Initialize the maze pointer so tick_maze doesn't fail
            extern struct rr_maze_declaration RR_MAZES[];
            arena->maze = &RR_MAZES[rr_biome_id_hell_creek];
        }
        // Don't initialize spatial hash - that's server-only
        // The client will receive arena updates from the server
    }
    else
    {
        // Arena exists, but make sure it has the maze pointer set
        // Find the arena entity (should be entity 1)
        for (EntityIdx i = 0; i < this->arena_count; ++i)
        {
            EntityIdx arena_id = this->arena_vector[i];
            struct rr_component_arena *arena = rr_simulation_get_arena(this, arena_id);
            if (arena)
            {
                arena->biome = rr_biome_id_hell_creek; // Always use hell creek
                if (arena->maze == NULL)
                {
                    extern struct rr_maze_declaration RR_MAZES[];
                    arena->maze = &RR_MAZES[rr_biome_id_hell_creek];
                }
            }
        }
    }
}
#endif

void rr_simulation_entity_create_with_id(struct rr_simulation *this,
                                         EntityIdx entity)
{
    this->entity_tracker[entity] = 1;
}

void rr_simulation_read_binary(struct rr_game *game, struct proto_bug *encoder)
{
    struct rr_simulation *this = game->simulation;
    EntityIdx id = 0;
    
    // Track which entities are being deleted in this message, so we can skip deletion
    // if they're immediately updated in the same message
    uint8_t deleted_this_message[RR_MAX_ENTITY_COUNT] = {0};
    uint32_t deletion_count = 0;
    
    while (1)
    {
        id = proto_bug_read_varuint(encoder, "entity deletion id");
        if (id == RR_NULL_ENTITY)
            break;
        deletion_count++;
        printf("<rr_client::read_binary::deletion_received::id=%u::count=%u>\n", (unsigned)id, deletion_count);
        assert(id < RR_MAX_ENTITY_COUNT);
        if (!this->entity_tracker[id])
        {
            printf("<rr_client::read_binary::deletion_for_nonexistent_entity::id=%u::skipping>\n", (unsigned)id);
            uint8_t type = proto_bug_read_uint8(encoder, "deletion type");
            continue; // Skip deletion if entity doesn't exist
        }
        uint8_t type = proto_bug_read_uint8(encoder, "deletion type");
        printf("<rr_client::read_binary::entity_deletion::id=%u::type=%u::tracker_before=%u>\n", 
               (unsigned)id, (unsigned)type, (unsigned)this->entity_tracker[id]);
        
        // Mark this entity as deleted in this message
        if (id < RR_MAX_ENTITY_COUNT)
            deleted_this_message[id] = 1;
        
        if (type)
        {
            struct rr_simulation *del_s = game->deletion_simulation;
            EntityIdx id_2 = rr_simulation_alloc_entity(del_s);
#define XX(COMPONENT, ID)                                                      \
    if (rr_simulation_has_##COMPONENT(this, id))                               \
    {                                                                          \
        struct rr_component_##COMPONENT *o =                                   \
            rr_simulation_get_##COMPONENT(this, id);                           \
        struct rr_component_##COMPONENT *c =                                   \
            rr_simulation_add_##COMPONENT(del_s, id_2);                        \
        memcpy(c, o, sizeof(struct rr_component_##COMPONENT));                 \
    }
            RR_FOR_EACH_COMPONENT
#undef XX
            if (rr_simulation_has_physical(del_s, id_2))
            {
                rr_simulation_get_physical(del_s, id_2)->deletion_type = type;
                if (rr_simulation_has_mob(this, id) &&
                    (rr_simulation_get_relations(this, id)->team ==
                         rr_simulation_team_id_mobs ||
                     rr_simulation_get_mob(this, id)->id == rr_mob_id_meteor))
                {
                    struct rr_component_mob *mob =
                        rr_simulation_get_mob(this, id);
                    ++game->cache.mob_kills[mob->id][mob->rarity];
                }
            }
        }
        __rr_simulation_pending_deletion_free_components(id, this);
        __rr_simulation_pending_deletion_unset_entity(id, this);
    }
    printf("<rr_client::read_binary::deletions_processed::count=%u>\n", deletion_count);

    // assuming that player info is written first (is it though)
    while (1)
    {
        id = proto_bug_read_varuint(encoder, "entity update id");
        if (id == RR_NULL_ENTITY)
            break;
        uint8_t is_creation = proto_bug_read_uint8(encoder, "upcreate");
        uint32_t component_flags =
            proto_bug_read_varuint(encoder, "entity component flags");
        
        printf("<rr_client::read_binary::entity_update::id=%u::is_creation=%u::flags=%u::tracker=%u::deleted_this_msg=%u>\n",
               (unsigned)id, (unsigned)is_creation, (unsigned)component_flags, 
               (unsigned)this->entity_tracker[id],
               (unsigned)(id < RR_MAX_ENTITY_COUNT ? deleted_this_message[id] : 0));

        // If entity was deleted in this same message, restore it instead of creating from scratch
        if (id < RR_MAX_ENTITY_COUNT && deleted_this_message[id] && this->entity_tracker[id] == 0)
        {
            printf("<rr_client::read_binary::entity_restored_after_deletion::id=%u>\n", (unsigned)id);
            deleted_this_message[id] = 0; // Clear the deletion flag since we're restoring it
        }

        // If server says it's a creation, or if entity doesn't exist on client, create it
        if (is_creation || this->entity_tracker[id] == 0)
        {
            if (is_creation)
            {
#ifndef RIVET_BUILD
                printf("create entity with id %d, components %d\n", id,
                       component_flags);
#endif
            }
            else
            {
                // Server says it's an update, but client doesn't have the entity
                // This happens when server's entities_in_view bitset is out of sync
                // Clear it from entities_in_view so next update sends it as a creation
                printf("<rr_client::read_binary::server_says_update_but_entity_missing::creating::id=%u>\n", (unsigned)id);
#ifdef SINGLE_PLAYER_BUILD
                // In single-player mode, we can access the server's player_info directly
                // Clear the entity from entities_in_view so it gets sent as creation next time
                if (game->player_info != NULL)
                {
                    extern void rr_bitset_unset(uint8_t *bitset, uint64_t bit);
                    rr_bitset_unset(game->player_info->entities_in_view, id);
                    printf("<rr_client::read_binary::cleared_entity_from_entities_in_view::id=%u>\n", (unsigned)id);
                }
#endif
            }
            this->entity_tracker[id] = 1 | component_flags;
#define XX(COMPONENT, ID)                                                      \
    if (component_flags & (1 << ID))                                           \
    {                                                                          \
        if (!rr_simulation_has_##COMPONENT(this, id))                          \
            rr_simulation_add_##COMPONENT(this, id);                           \
    }
            RR_FOR_EACH_COMPONENT
#undef XX
        }
        else if ((component_flags >> 1) != (this->entity_tracker[id] >> 1))
        {
            // Protocol error: entity component flags don't match
            // In single-player mode, this might happen if the server sends an update
            // for an entity that the client hasn't created yet. Treat it as a creation.
            if (this->entity_tracker[id] == 0)
            {
                printf("<rr_client::read_binary::treating_update_as_creation::entity=%u::flags=%u>\n",
                       (unsigned)id, (unsigned)component_flags);
                this->entity_tracker[id] = 1 | component_flags;
#define XX(COMPONENT, ID)                                                      \
    if (component_flags & (1 << ID))                                           \
        rr_simulation_add_##COMPONENT(this, id);
                RR_FOR_EACH_COMPONENT
#undef XX
            }
            else
            {
                // Component flags don't match - update client to match server
                // This can happen if components are added/removed on the server
                uint32_t old_flags = this->entity_tracker[id];
                uint32_t missing_components = (component_flags & ~old_flags) >> 1;
                
                printf("<rr_client::read_binary::component_mismatch::entity=%u::expected=%u::got=%u::updating_to_match_server>\n",
                       (unsigned)id, (unsigned)(old_flags >> 1), (unsigned)(component_flags >> 1));
                
                // Add any missing components
                if (missing_components != 0)
                {
                    uint32_t temp_flags = missing_components;
#define XX(COMPONENT, ID)                                                      \
    if (temp_flags & (1 << ID))                                                \
    {                                                                          \
        if (!rr_simulation_has_##COMPONENT(this, id))                          \
            rr_simulation_add_##COMPONENT(this, id);                           \
        temp_flags &= ~(1 << ID);                                              \
    }
                    RR_FOR_EACH_COMPONENT
#undef XX
                }
                
                // Update entity tracker to match server
                this->entity_tracker[id] = 1 | component_flags;
            }
        }

#define XX(COMPONENT, ID)                                                      \
    if (component_flags & (1 << ID))                                           \
        rr_component_##COMPONENT##_read(                                       \
            rr_simulation_get_##COMPONENT(this, id), encoder);
        RR_FOR_EACH_COMPONENT
#undef XX
        
        // In single-player mode, ensure arena components have their maze pointer set
        // (pointers aren't serialized, so we need to restore them after reading)
#ifdef SINGLE_PLAYER_BUILD
        if (component_flags & (1 << 12)) // Arena component ID is 12
        {
            struct rr_component_arena *arena = rr_simulation_get_arena(this, id);
            if (arena && (arena->maze == NULL || arena->biome != rr_biome_id_hell_creek))
            {
                extern struct rr_maze_declaration RR_MAZES[];
                arena->maze = &RR_MAZES[rr_biome_id_hell_creek]; // Always use hell creek
                arena->biome = rr_biome_id_hell_creek; // Ensure biome is also set
            }
        }
#endif
    }
    EntityIdx pinfo_id = proto_bug_read_varuint(encoder, "pinfo id");
    printf("<rr_client::read_binary::pinfo_id=%u>\n", (unsigned)pinfo_id);
    
    // Check if entity exists and has player_info component
    if (!rr_simulation_has_entity(this, pinfo_id))
    {
        printf("<rr_client::read_binary::ERROR_pinfo_entity_does_not_exist::pinfo_id=%u>\n", (unsigned)pinfo_id);
        game->player_info = NULL;
    }
    else if (!rr_simulation_has_player_info(this, pinfo_id))
    {
        printf("<rr_client::read_binary::ERROR_pinfo_entity_has_no_player_info_component::pinfo_id=%u>\n", (unsigned)pinfo_id);
        game->player_info = NULL;
    }
    else
    {
        game->player_info = rr_simulation_get_player_info(this, pinfo_id);
        if (game->player_info == NULL)
        {
            printf("<rr_client::read_binary::player_info_is_null::pinfo_id=%u>\n", (unsigned)pinfo_id);
        }
        else
        {
            printf("<rr_client::read_binary::player_info_set::pinfo_id=%u::flower_id=%u::camera_x=%f::camera_y=%f::camera_fov=%f::squad_pos=%u::game_squad_pos=%u>\n", 
                   (unsigned)pinfo_id, 
                   (unsigned)game->player_info->flower_id,
                   game->player_info->camera_x,
                   game->player_info->camera_y,
                   game->player_info->camera_fov,
                   (unsigned)game->player_info->squad_pos,
                   (unsigned)game->squad.squad_pos);
            // Verify that this player_info belongs to this client
            // If squad_pos doesn't match, the data might be corrupted
            if (game->player_info->squad_pos != game->squad.squad_pos)
            {
                printf("<rr_client::read_binary::WARNING_squad_pos_mismatch::player_info_squad_pos=%u::game_squad_pos=%u::data_may_be_corrupted>\n",
                       (unsigned)game->player_info->squad_pos,
                       (unsigned)game->squad.squad_pos);
                // Clear from entities_in_view so next update sends it as creation
#ifdef SINGLE_PLAYER_BUILD
                extern void rr_bitset_unset(uint8_t *bitset, uint64_t bit);
                rr_bitset_unset(game->player_info->entities_in_view, pinfo_id);
                printf("<rr_client::read_binary::cleared_player_info_from_entities_in_view::pinfo_id=%u>\n", (unsigned)pinfo_id);
#endif
            }
            // Check if flower exists
            if (game->player_info->flower_id != RR_NULL_ENTITY)
            {
                EntityIdx flower_id = (EntityIdx)game->player_info->flower_id;
                if (!rr_simulation_has_entity(this, flower_id))
                {
                    printf("<rr_client::read_binary::WARNING_flower_entity_does_not_exist::flower_id=%u>\n", (unsigned)flower_id);
                }
                else if (!rr_simulation_has_physical(this, flower_id))
                {
                    printf("<rr_client::read_binary::WARNING_flower_has_no_physical_component::flower_id=%u>\n", (unsigned)flower_id);
                }
                else
                {
                    struct rr_component_physical *physical = rr_simulation_get_physical(this, flower_id);
                    printf("<rr_client::read_binary::flower_exists::flower_id=%u::x=%f::y=%f::radius=%f>\n",
                           (unsigned)flower_id, physical->x, physical->y, physical->radius);
                }
            }
            else
            {
                printf("<rr_client::read_binary::WARNING_flower_id_is_null>\n");
            }
        }
    }
    this->game_over = proto_bug_read_uint8(encoder, "game over");
    this->updated_this_tick = 1;
    printf("<rr_client::read_binary::complete::updated_this_tick=1>\n");
}

#ifndef SINGLE_PLAYER_BUILD
// In non-single-player mode, client provides its own version
void rr_simulation_tick(struct rr_simulation *this, float delta)
{
    rr_simulation_create_component_vectors(this);
    rr_system_interpolation_tick(this, delta);
}
#endif
// In single-player mode, server's version is used (defined in Server/Simulation.c)

void rr_deletion_simulation_tick(struct rr_simulation *this, float delta)
{
    rr_bitset_for_each_bit(
        this->pending_deletions,
        this->pending_deletions + RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT), this,
        __rr_simulation_pending_deletion_free_components);
    rr_bitset_for_each_bit(this->pending_deletions,
                           this->pending_deletions +
                               RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT),
                           this, __rr_simulation_pending_deletion_unset_entity);
    memset(this->pending_deletions, 0, RR_BITSET_ROUND(RR_MAX_ENTITY_COUNT));
    rr_simulation_create_component_vectors(this);
    rr_system_deletion_animation_tick(this, delta);
}

// Both client and server need this function
// In single-player mode, both versions exist but they're the same implementation
EntityIdx rr_simulation_alloc_entity(struct rr_simulation *this)
{
    for (EntityIdx i = 1; i < RR_MAX_ENTITY_COUNT; i++)
    {
        if (!rr_simulation_has_entity(this, i))
        {
            this->entity_tracker[i] = 1;
            // no more manual
#ifndef NDEBUG
            printf("<rr_simulation::entity_create::%d>\n", i);
#endif
            return i;
        }
    }
    RR_UNREACHABLE("ran out of entity ids");
}
