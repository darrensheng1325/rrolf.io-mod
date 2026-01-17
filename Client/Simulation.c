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

// Unified simulation initialization - works for both single-player and multiplayer
// In single-player mode, Server/Simulation.c provides rr_simulation_init
// In multiplayer mode, this provides a simple initialization
#ifndef SINGLE_PLAYER_BUILD
void rr_simulation_init(struct rr_simulation *this)
{
    memset(this, 0, sizeof *this);
}
#endif
// No special arena initialization needed - server handles arena creation
// and client receives updates through normal protocol

void rr_simulation_entity_create_with_id(struct rr_simulation *this,
                                         EntityIdx entity)
{
    this->entity_tracker[entity] = 1;
}

void rr_simulation_read_binary(struct rr_game *game, struct proto_bug *encoder)
{
    struct rr_simulation *this = game->simulation;
    EntityIdx id = 0;
    while (1)
    {
        id = proto_bug_read_varuint(encoder, "entity deletion id");
        if (id == RR_NULL_ENTITY)
            break;
        assert(id < RR_MAX_ENTITY_COUNT);
        assert(this->entity_tracker[id]);
        uint8_t type = proto_bug_read_uint8(encoder, "deletion type");
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

    // assuming that player info is written first (is it though)
    while (1)
    {
        id = proto_bug_read_varuint(encoder, "entity update id");
        if (id == RR_NULL_ENTITY)
            break;
        uint8_t is_creation = proto_bug_read_uint8(encoder, "upcreate");
        uint64_t encoder_pos_before_flags = encoder->current - encoder->start;
        uint32_t component_flags =
            proto_bug_read_varuint(encoder, "entity component flags");
        uint64_t encoder_pos_after_flags = encoder->current - encoder->start;
        if (component_flags & (1 << 3) || component_flags & (1 << 1)) // physical or player_info
        {
            // printf("<rr_client::read_entity::id=%u::is_creation=%u::component_flags=0x%x::encoder_before_flags=%llu::encoder_after_flags=%llu::flags_bytes=%llu>\n",
            //        (unsigned)id, (unsigned)is_creation, (unsigned)component_flags,
            //        (unsigned long long)encoder_pos_before_flags,
            //        (unsigned long long)encoder_pos_after_flags,
            //        (unsigned long long)(encoder_pos_after_flags - encoder_pos_before_flags));
        }

        uint64_t encoder_pos_before_entity = encoder->current - encoder->start;
        if (is_creation)
        {
#ifndef RIVET_BUILD
            printf("create entity with id %d, components %d\n", id,
                   component_flags);
#endif
            printf("<rr_client::entity_creation::id=%u::component_flags=0x%x::encoder_pos=%llu>\n",
                   (unsigned)id, (unsigned)component_flags, (unsigned long long)encoder_pos_before_entity);
            this->entity_tracker[id] = 1 | component_flags;
#define XX(COMPONENT, ID)                                                      \
    if (component_flags & (1 << ID))                                           \
        rr_simulation_add_##COMPONENT(this, id);
            RR_FOR_EACH_COMPONENT
#undef XX
        }
        else 
        {
            uint32_t expected_flags = this->entity_tracker[id];
            if (expected_flags == 0)
            {
                // Entity doesn't exist on client but server says it's an update
                // This means the entity was created but we missed the creation message
                // Treat it as a creation and add all components
                printf("<rr_client::server_says_update_but_entity_missing::creating::id=%u::component_flags=0x%x::encoder_pos=%llu>\n",
                       (unsigned)id, (unsigned)component_flags, (unsigned long long)encoder_pos_before_entity);
                // First mark entity as existing (bit 0 = 1)
                this->entity_tracker[id] = 1;
                // Then add all components that the server is sending
#define XX(COMPONENT, ID)                                                      \
    if (component_flags & (1 << ID))                                           \
        rr_simulation_add_##COMPONENT(this, id);
                RR_FOR_EACH_COMPONENT
#undef XX
                // Verify the tracker matches what we expect
                if (this->entity_tracker[id] != (1 | component_flags))
                {
                    printf("<rr_client::entity_creation_mismatch::id=%u::expected=0x%x::got=0x%x>\n",
                           (unsigned)id, (unsigned)(1 | component_flags), (unsigned)this->entity_tracker[id]);
                    this->entity_tracker[id] = 1 | component_flags;
                }
                printf("<rr_client::entity_created_from_update::id=%u::flags=0x%x>\n",
                       (unsigned)id, (unsigned)this->entity_tracker[id]);
            }
            else if ((component_flags >> 1) != (expected_flags >> 1))
            {
                printf(
                    "protocol error: entity %d misaligned: expected 0x%x (>>1=%u) but got 0x%x (>>1=%u)::encoder_pos_before_flags=%llu::encoder_pos_after_flags=%llu::flags_bytes=%llu\n",
                    id, expected_flags, (unsigned)(expected_flags >> 1), component_flags, (unsigned)(component_flags >> 1),
                    (unsigned long long)encoder_pos_before_flags, (unsigned long long)encoder_pos_after_flags,
                    (unsigned long long)(encoder_pos_after_flags - encoder_pos_before_flags));
                // Component flags changed - need to add missing components
                // Add any components that are in component_flags but not in expected_flags
                uint32_t missing_components = component_flags & ~expected_flags;
                if (missing_components != 0)
                {
                    printf("<rr_client::adding_missing_components::id=%u::missing=0x%x>\n",
                           (unsigned)id, (unsigned)missing_components);
#define XX(COMPONENT, ID)                                                      \
    if (missing_components & (1 << ID))                                        \
        rr_simulation_add_##COMPONENT(this, id);
                    RR_FOR_EACH_COMPONENT
#undef XX
                }
                // Update entity_tracker to match what server sent
                this->entity_tracker[id] = 1 | component_flags;
                printf("<rr_client::entity_tracker_fixed::id=%u::new_flags=0x%x>\n",
                       (unsigned)id, (unsigned)this->entity_tracker[id]);
            }
        }

#define XX(COMPONENT, ID)                                                      \
    if (component_flags & (1 << ID))                                           \
        rr_component_##COMPONENT##_read(                                       \
            rr_simulation_get_##COMPONENT(this, id), encoder);
        RR_FOR_EACH_COMPONENT
#undef XX
    }
    game->player_info = rr_simulation_get_player_info(
        this, proto_bug_read_varuint(encoder, "pinfo id"));
    this->game_over = proto_bug_read_uint8(encoder, "game over");
    this->updated_this_tick = 1;
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
