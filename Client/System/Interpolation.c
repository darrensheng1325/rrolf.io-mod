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

#include <Client/System/Interpolation.h>

#include <math.h>

#include <Shared/Entity.h>
#include <Shared/SimulationCommon.h>
#include <Shared/Vector.h>

struct function_captures
{
    float delta;
    struct rr_simulation *simulation;
};

void system_interpolation_for_each_function(EntityIdx entity, void *_captures)
{
    struct function_captures *captures = _captures;
    struct rr_simulation *this = captures->simulation;
    float delta = captures->delta;
    if (rr_simulation_has_physical(this, entity))
    {
        struct rr_component_physical *physical =
            rr_simulation_get_physical(this, entity);
        // REMOVED INTERPOLATION: Copy actual values directly to lerp values
        physical->lerp_x = physical->x;
        physical->lerp_y = physical->y;
        physical->lerp_velocity.x = physical->velocity.x;
        physical->lerp_velocity.y = physical->velocity.y;
        physical->lerp_angle = physical->angle;
        physical->lerp_radius = physical->radius;
        if (rr_simulation_has_mob(this, entity))
        {
            if (physical->turning_animation == 0)
                physical->turning_animation = physical->angle;
            physical->turning_animation = physical->angle;

            physical->animation_timer +=
                (2 * (physical->parent_id % 2) - 1) * delta *
                (fmin(rr_vector_get_magnitude(&physical->lerp_velocity), 20) *
                     0.5 +
                 1) *
                2;
        }
        else
            physical->animation_timer += 0.5;
    }

    if (rr_simulation_has_flower(this, entity))
    {
        struct rr_component_flower *flower =
            rr_simulation_get_flower(this, entity);
        struct rr_component_physical *physical =
            rr_simulation_get_physical(this, entity);
        flower->eye_x = cosf(flower->eye_angle - physical->angle) * 3;
        flower->eye_y = sinf(flower->eye_angle - physical->angle) * 3;
        // REMOVED INTERPOLATION: Copy actual values directly to lerp values
        flower->lerp_eye_x = flower->eye_x;
        flower->lerp_eye_y = flower->eye_y;
        if (flower->face_flags & 1)
            flower->lerp_mouth = 5;
        else if (flower->face_flags & 2)
            flower->lerp_mouth = 8;
        else
            flower->lerp_mouth = 15;
    }

    if (rr_simulation_has_player_info(this, entity))
    {
        struct rr_component_player_info *player_info =
            rr_simulation_get_player_info(this, entity);
        // REMOVED INTERPOLATION: Copy actual values directly to lerp values
        player_info->lerp_camera_fov = player_info->camera_fov;
        player_info->lerp_camera_x = player_info->camera_x;
        player_info->lerp_camera_y = player_info->camera_y;
    }

    if (rr_simulation_has_health(this, entity))
    {
        struct rr_component_health *health =
            rr_simulation_get_health(this, entity);
        health->damage_animation =
            rr_lerp(health->damage_animation, 0, 5 * delta);
        if (health->flags & 2)
        {
            if (health->damage_animation < 0.25)
                health->damage_animation = 1;
        }
        // REMOVED INTERPOLATION: Copy actual values directly to lerp values
        health->lerp_health = health->health;
    }
}

void rr_system_interpolation_tick(struct rr_simulation *simulation, float delta)
{
    struct function_captures captures;
    captures.simulation = simulation;
    captures.delta = delta;
    rr_simulation_for_each_entity(simulation, &captures,
                                  system_interpolation_for_each_function);
    simulation->updated_this_tick = 0;
}
