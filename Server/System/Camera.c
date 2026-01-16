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

#include <Server/System/System.h>

#include <Server/Simulation.h>
#include <Shared/Component/PlayerInfo.h>
#include <Shared/Entity.h>
#include <Shared/Vector.h>
#include <math.h>
#include <stdio.h>

void rr_system_camera_tick(struct rr_simulation *this)
{
    for (EntityIdx i = 0; i < this->player_info_count; ++i)
    {
        struct rr_component_player_info *player_info =
            rr_simulation_get_player_info(this, this->player_info_vector[i]);
        if (player_info->flower_id != RR_NULL_ENTITY)
        {
            struct rr_component_physical *physical =
                rr_simulation_get_physical(this, player_info->flower_id);
            // Check for actual corruption (NaN/Inf) instead of large but valid positions
            if (isnan(physical->x) || isnan(physical->y) || isinf(physical->x) || isinf(physical->y))
            {
                printf("<rr_server::camera_tick::CORRUPTED_POSITION::flower_id=%u::x=%f::y=%f>\n",
                       (unsigned)player_info->flower_id, physical->x, physical->y);
            }
            // Store old values to check if they changed
            float old_camera_x = player_info->camera_x;
            float old_camera_y = player_info->camera_y;
            
            rr_component_player_info_set_camera_x(player_info, physical->x);
            rr_component_player_info_set_camera_y(player_info, physical->y);
            // Note: physical->arena is RR_SERVER_ONLY, so it exists on server
            rr_component_player_info_set_arena(player_info, physical->arena);
            
            // If values didn't change, the setter won't set protocol_state
            // Force protocol_state to be set so camera position is sent to client
            // This ensures player_info updates are sent even when player isn't moving
            // Using bit values: camera_x = 0b0000100 (4), camera_y = 0b1000000 (64)
            if (old_camera_x == player_info->camera_x && old_camera_y == player_info->camera_y)
            {
                player_info->protocol_state |= 0b0000100 | 0b1000000; // camera_x | camera_y
            }
            continue;
        }
        // tempfix
        continue;
        uint8_t has_seed = 0;
        for (EntityIdx j = 0; j < this->petal_count; ++j)
        {
            struct rr_component_petal *petal =
                rr_simulation_get_petal(this, this->petal_vector[j]);
            if (petal->id != rr_petal_id_seed || petal->detached == 0)
                continue;
            struct rr_component_physical *physical =
                rr_simulation_get_physical(this, this->petal_vector[j]);
            rr_component_player_info_set_camera_x(player_info, physical->x);
            rr_component_player_info_set_camera_y(player_info, physical->y);
            has_seed = 1;
            break;
        }
        if (this->flower_count > 0 && !has_seed)
        {
            struct rr_component_physical *physical =
                rr_simulation_get_physical(this, this->flower_vector[0]);
            rr_component_player_info_set_camera_x(player_info, physical->x);
            rr_component_player_info_set_camera_y(player_info, physical->y);
        }
    }
}
