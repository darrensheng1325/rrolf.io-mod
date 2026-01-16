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

#include <Client/Renderer/ComponentRender.h>

#include <math.h>
#include <assert.h>

#include <Client/Assets/Render.h>
#include <Client/Assets/RenderFunctions.h>
#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/Renderer.h>
#include <Client/Simulation.h>
#include <Shared/Crypto.h>

void render_background(struct rr_component_player_info *player_info,
                       struct rr_game *this)
{
    if (player_info->arena == 0)
        return;
    struct rr_renderer *renderer = this->renderer;
    double scale = player_info->lerp_camera_fov * renderer->scale;
    double leftX = player_info->lerp_camera_x - renderer->width / (2 * scale);
    double rightX = player_info->lerp_camera_x + renderer->width / (2 * scale);
    double topY = player_info->lerp_camera_y - renderer->height / (2 * scale);
    double bottomY =
        player_info->lerp_camera_y + renderer->height / (2 * scale);

#define GRID_SIZE (512.0f)
    double newLeftX = floorf(leftX / GRID_SIZE) * GRID_SIZE;
    double newTopY = floorf(topY / GRID_SIZE) * GRID_SIZE;
    // rr_renderer_scale(renderer, scale);
    for (; newLeftX < rightX; newLeftX += GRID_SIZE)
    {
        for (double currY = newTopY; currY < bottomY; currY += GRID_SIZE)
        {
            uint32_t tile_index =
                rr_get_hash((uint32_t)(((newLeftX + 8192) / GRID_SIZE + 1) *
                                       ((currY + 8192) / GRID_SIZE + 2))) %
                3;
            struct rr_renderer_context_state state;
            rr_renderer_context_state_init(renderer, &state);
            rr_renderer_translate(renderer, newLeftX + GRID_SIZE / 2,
                                  currY + GRID_SIZE / 2);
            rr_renderer_scale(renderer, (GRID_SIZE + 2) / 256);
            if (this->selected_biome == 0)
                rr_renderer_draw_tile_hell_creek(renderer, tile_index);
            else
                rr_renderer_draw_tile_garden(renderer, tile_index);
            rr_renderer_context_state_free(renderer, &state);
        }
    }

#undef GRID_SIZE
    struct rr_component_arena *arena =
        rr_simulation_get_arena(this->simulation, player_info->arena);
    assert(arena != NULL);
    float grid_size = RR_MAZES[arena->biome].grid_size;
    uint32_t maze_dim = RR_MAZES[arena->biome].maze_dim;
    struct rr_maze_grid *grid = RR_MAZES[arena->biome].maze;
    rr_renderer_set_fill(renderer, 0xff000000);
    rr_renderer_set_global_alpha(renderer, 0.5f);
    int32_t nx = floorf(leftX / grid_size);
    int32_t ny = floorf(topY / grid_size);
#define TILE_SIZE (256.0f)
    double tileLeftX = floorf(leftX / TILE_SIZE) * TILE_SIZE;
    double tileTopY = floorf(topY / TILE_SIZE) * TILE_SIZE;
    for (; tileLeftX < rightX; tileLeftX += TILE_SIZE)
    {
        for (double tileY = tileTopY; tileY < bottomY; tileY += TILE_SIZE)
        {
            int32_t tile_nx = floorf(tileLeftX / grid_size);
            int32_t tile_ny = floorf(tileY / grid_size);
            uint8_t tile =
                (tile_nx < 0 || tile_ny < 0 || tile_nx >= maze_dim || tile_ny >= maze_dim)
                    ? 0
                    : grid[tile_ny * maze_dim + tile_nx].value;
            struct rr_renderer_context_state tile_state;
            rr_renderer_context_state_init(renderer, &tile_state);
            rr_renderer_translate(renderer, tileLeftX + TILE_SIZE / 2,
                                  tileY + TILE_SIZE / 2);
            rr_renderer_set_global_alpha(renderer, 1.0f);
            if (tile == 1)
            {
                rr_grass_draw(renderer);
            }
            else if (tile == 2)
            {
                rr_water_draw(renderer);
            }
            else if (tile == 0)
            {
                rr_dirt_draw(renderer);
            }
            else
            {
                rr_grass_draw(renderer);
            }
            rr_renderer_context_state_free(renderer, &tile_state);
        }
    }
#undef TILE_SIZE
}

void rr_component_arena_render(EntityIdx entity, struct rr_game *this,
                               struct rr_simulation *simulation)
{
    struct rr_renderer_context_state state2;
    struct rr_component_player_info *player_info = this->player_info;

    struct rr_component_arena *arena =
        rr_simulation_get_arena(this->simulation, player_info->arena);
    rr_renderer_context_state_init(this->renderer, &state2);
    render_background(player_info, this);

    rr_renderer_context_state_free(this->renderer, &state2);
}