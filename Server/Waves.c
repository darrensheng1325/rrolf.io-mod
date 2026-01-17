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

#include <Server/Waves.h>

#include <math.h>
#include <stdlib.h>
#include <assert.h>

#include <Server/Simulation.h>
#include <Shared/StaticData.h>
#include <Shared/Utilities.h>

uint32_t get_spawn_rarity(float difficulty)
{
    if (difficulty < 1)
        difficulty = 1;
    double rarity_seed = rr_frand();
    uint32_t rarity_cap = rr_rarity_id_common + (difficulty + 7) / 8;
    if (rarity_cap > rr_rarity_id_ultimate)
        rarity_cap = rr_rarity_id_ultimate;
    uint32_t rarity = rarity_cap >= 2 ? rarity_cap - 2 : 0;
    for (; rarity < rarity_cap; ++rarity)
        if (pow(1 - (1 - RR_MOB_WAVE_RARITY_COEFFICIENTS[rarity + 1]) * 0.3,
                pow(1.5, difficulty)) >= rarity_seed)
            break;
    return rarity;
}

uint8_t get_spawn_id(uint8_t biome, struct rr_maze_grid *zone)
{
    // Default to Hell Creek coefficients, only use Garden for explicit garden biome
    double *table = (biome == rr_biome_id_garden) ? RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS
                                                  : RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS;
    
    // The table contains cumulative probabilities (normalized to [0, 1])
    // after init_game_coefficients() processes them.
    // Generate a random value between 0 and 1
    double random_value = rr_frand();
    
    // Debug: Show coefficient values and selection (first 20 spawns only)
    static int debug_count = 0;
    if (debug_count++ < 20)
    {
        printf("get_spawn_id[%d]: biome=%u, random=%.6f, table[1]=%.6f, table[4]=%.6f, table[11]=%.6f, table[12]=%.6f, table[19]=%.6f\n",
               debug_count - 1, biome, random_value, 
               table[1], table[4], table[11], table[12], table[19]);
    }
    
    // Find the first mob where cumulative probability >= random_value
    // Skip mobs with zero weight (they have the same cumulative probability as the previous mob)
    double prev_cumulative = -1.0;
    for (uint8_t id = 0; id < rr_mob_id_max; ++id)
    {
        // Skip mobs with zero weight (cumulative probability equals previous)
        if (table[id] > prev_cumulative && random_value <= table[id])
        {
            if (debug_count <= 20)
                printf("  -> Selected mob_id=%u (%s), prev_cum=%.6f, table[%u]=%.6f\n",
                       id, RR_MOB_NAMES[id], prev_cumulative, id, table[id]);
            return id;
        }
        prev_cumulative = table[id];
    }
    
    // Fallback: find the last mob with non-zero weight
    for (int8_t id = rr_mob_id_max - 1; id >= 0; --id)
    {
        if (id == 0 || table[id] > table[id - 1])
            return id;
    }
    
    // Final fallback: return first mob (shouldn't reach here if coefficients are correct)
    return 0;
}

int should_spawn_at(uint8_t id, uint8_t rarity)
{
    return rarity >= RR_MOB_DATA[id].min_rarity &&
           rarity <= RR_MOB_DATA[id].max_rarity;
}