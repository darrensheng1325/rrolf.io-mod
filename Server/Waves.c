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
    // Use explicit biome enum comparison instead of == 0
    double *table = (biome == rr_biome_id_hell_creek) ? RR_HELL_CREEK_MOB_ID_RARITY_COEFFICIENTS
                                                      : RR_GARDEN_MOB_ID_RARITY_COEFFICIENTS;
    
    // Calculate total weight sum
    double total_weight = 0.0;
    for (uint8_t i = 0; i < rr_mob_id_max; ++i)
    {
        total_weight += table[i];
    }
    
    // If no weights, return first mob (shouldn't happen)
    if (total_weight <= 0.0)
        return 0;
    
    // Generate random value between 0 and total_weight
    double random_value = rr_frand() * total_weight;
    
    // Select mob based on weighted random
    double cumulative = 0.0;
    for (uint8_t id = 0; id < rr_mob_id_max; ++id)
    {
        cumulative += table[id];
        if (random_value <= cumulative)
            return id;
    }
    
    // Fallback (shouldn't reach here)
    return rr_mob_id_max - 1;
}

int should_spawn_at(uint8_t id, uint8_t rarity)
{
    return rarity >= RR_MOB_DATA[id].min_rarity &&
           rarity <= RR_MOB_DATA[id].max_rarity;
}