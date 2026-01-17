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

#include <Shared/Component/Physical.h>

#include <math.h>
#include <stdio.h>
#include <string.h>

#include <Shared/pb.h>

enum
{
    state_flags_y = 0b00001,
    state_flags_radius = 0b00010,
    state_flags_angle = 0b00100,
    state_flags_x = 0b01000,
    state_flags_all = 0b01111
};

#define FOR_EACH_PUBLIC_FIELD                                                  \
    X(angle, float32)                                                          \
    X(radius, float32)                                                         \
    X(x, float32)                                                              \
    X(y, float32)

void rr_component_physical_init(struct rr_component_physical *this,
                                struct rr_simulation *simulation)
{
    memset(this, 0, sizeof *this);
    RR_SERVER_ONLY(this->mass = 1;)
    RR_SERVER_ONLY(this->acceleration_scale = 1;)
    RR_SERVER_ONLY(this->knockback_scale = 1;)
    RR_SERVER_ONLY(this->aggro_range_multiplier = 1;)
}

void rr_component_physical_free(struct rr_component_physical *this,
                                struct rr_simulation *simulation)
{
}

#ifdef RR_SERVER
void rr_component_physical_write(struct rr_component_physical *this,
                                 struct proto_bug *encoder, int is_creation,
                                 struct rr_component_player_info *client)
{
    uint64_t state = this->protocol_state | (state_flags_all * is_creation);
    uint64_t encoder_pos_before = encoder->current - encoder->start;
    proto_bug_write_varuint(encoder, state, "physical component state");
    uint64_t encoder_pos_after_state = encoder->current - encoder->start;
    
    // Debug: log what we're writing
    if (state & state_flags_x || state & state_flags_y)
    {
        // printf("<rr_server::physical_write::state=0x%llx::x=%f::y=%f::encoder_before=%llu::encoder_after_state=%llu>\n",
        //        (unsigned long long)state, this->x, this->y,
        //        (unsigned long long)encoder_pos_before,
        //        (unsigned long long)encoder_pos_after_state);
    }
#define X(NAME, TYPE) RR_ENCODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
    uint64_t encoder_pos_after = encoder->current - encoder->start;
    if (state & state_flags_x || state & state_flags_y)
    {
        // printf("<rr_server::physical_write::complete::bytes_written=%llu>\n",
        //        (unsigned long long)(encoder_pos_after - encoder_pos_before));
    }
}

RR_DEFINE_PUBLIC_FIELD(physical, float, x)
RR_DEFINE_PUBLIC_FIELD(physical, float, y)
RR_DEFINE_PUBLIC_FIELD(physical, float, angle)
RR_DEFINE_PUBLIC_FIELD(physical, float, radius)
#endif

#ifdef RR_CLIENT
void rr_component_physical_read(struct rr_component_physical *this,
                                struct proto_bug *encoder)
{
    uint64_t encoder_pos_before = encoder->current - encoder->start;
    float x_before = this->x;
    float y_before = this->y;
    uint64_t state =
        proto_bug_read_varuint(encoder, "physical component state");
    uint64_t encoder_pos_after_state = encoder->current - encoder->start;
    
    // Debug: log what we're reading
    if (state & state_flags_x || state & state_flags_y)
    {
        // printf("<rr_client::physical_read::state=0x%llx::encoder_before=%llu::encoder_after_state=%llu::state_bytes=%llu::x_before=%f::y_before=%f>\n",
        //        (unsigned long long)state,
        //        (unsigned long long)encoder_pos_before,
        //        (unsigned long long)encoder_pos_after_state,
        //        (unsigned long long)(encoder_pos_after_state - encoder_pos_before),
        //        x_before, y_before);
    }
#define X(NAME, TYPE) RR_DECODE_PUBLIC_FIELD(NAME, TYPE);
    FOR_EACH_PUBLIC_FIELD
#undef X
    
    uint64_t encoder_pos_after = encoder->current - encoder->start;
    if (state & state_flags_x || state & state_flags_y)
    {
        // printf("<rr_client::physical_read::complete::x_before=%f::x_after=%f::y_before=%f::y_after=%f::bytes_read=%llu>\n",
        //        x_before, this->x, y_before, this->y,
        //        (unsigned long long)(encoder_pos_after - encoder_pos_before));
        // Check if position was set to 0 when it shouldn't be
        if ((state & state_flags_x) && this->x == 0 && x_before != 0)
        {
            printf("<rr_client::physical_read::WARNING_X_TELEPORTED_TO_ZERO::x_before=%f::x_after=%f>\n",
                   x_before, this->x);
        }
        if ((state & state_flags_y) && this->y == 0 && y_before != 0)
        {
            printf("<rr_client::physical_read::WARNING_Y_TELEPORTED_TO_ZERO::y_before=%f::y_after=%f>\n",
                   y_before, this->y);
        }
        // Check for corruption
        if (isnan(this->x) || isnan(this->y) || isinf(this->x) || isinf(this->y))
        {
            printf("<rr_client::physical_read::CORRUPTION_DETECTED::x=%f::y=%f>\n", this->x, this->y);
        }
    }
}
#endif