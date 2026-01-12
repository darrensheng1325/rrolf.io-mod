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

#include <Client/Assets/RenderFunctions.h>

#include <math.h>
#include <stdint.h>

#include <Client/Renderer/Renderer.h>
#include <Shared/StaticData.h>
#include <Shared/Utilities.h>

// Helper functions
static uint32_t hsv_to_rgb(uint32_t color, float saturation_mult)
{
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = color & 0xff;
    
    // Simple HSV adjustment - multiply saturation
    float max = fmaxf(fmaxf(r, g), b) / 255.0f;
    float min = fminf(fminf(r, g), b) / 255.0f;
    float delta = max - min;
    
    if (delta < 0.001f)
        return color;
    
    float s = delta / max;
    s *= saturation_mult;
    s = fminf(1.0f, fmaxf(0.0f, s));
    
    float new_delta = max * s;
    float c = new_delta;
    float x = c * (1.0f - fabsf(fmodf((r / 255.0f - g / 255.0f) / delta, 6.0f) - 3.0f) - 1.0f);
    float m = max - c;
    
    float new_r = 0, new_g = 0, new_b = 0;
    if (delta == 0)
    {
        new_r = new_g = new_b = max;
    }
    else
    {
        float h = 0;
        if (max == r / 255.0f)
            h = fmodf((g / 255.0f - b / 255.0f) / delta, 6.0f);
        else if (max == g / 255.0f)
            h = (b / 255.0f - r / 255.0f) / delta + 2.0f;
        else
            h = (r / 255.0f - g / 255.0f) / delta + 4.0f;
        
        h *= 60.0f;
        float c = new_delta;
        float x = c * (1.0f - fabsf(fmodf(h / 60.0f, 2.0f) - 1.0f));
        
        if (h < 60)
        {
            new_r = c; new_g = x; new_b = 0;
        }
        else if (h < 120)
        {
            new_r = x; new_g = c; new_b = 0;
        }
        else if (h < 180)
        {
            new_r = 0; new_g = c; new_b = x;
        }
        else if (h < 240)
        {
            new_r = 0; new_g = x; new_b = c;
        }
        else if (h < 300)
        {
            new_r = x; new_g = 0; new_b = c;
        }
        else
        {
            new_r = c; new_g = 0; new_b = x;
        }
        
        new_r = (new_r + m) * 255.0f;
        new_g = (new_g + m) * 255.0f;
        new_b = (new_b + m) * 255.0f;
    }
    
    return 0xff000000 | ((uint32_t)new_r << 16) | ((uint32_t)new_g << 8) | (uint32_t)new_b;
}

// Simple seed generator
struct seed_gen
{
    uint32_t seed;
};

static float seed_next(struct seed_gen *gen)
{
    gen->seed = gen->seed * 1103515245 + 12345;
    return ((float)(gen->seed & 0x7fffffff)) / 2147483647.0f;
}

static float seed_binext(struct seed_gen *gen)
{
    gen->seed = gen->seed * 1103515245 + 12345;
    return (gen->seed & 1) ? 1.0f : -1.0f;
}

// Flower colors (placeholder - adjust as needed)
static uint32_t FLOWER_COLORS[] = {
    0xffffe763, 0xffeb4034, 0xff905db0, 0xff32a852, 0xff777777,
    0xff8ac255, 0xffd4c66e, 0xffc69a2d, 0xffb58500, 0xff454545
};

// Helper macro for SET_BASE_COLOR
// Note: flags parameter in C++ code was different (flower color flag), 
// but in C code flags is for cache/friendly/centipede, so we always use set_color
#define SET_BASE_COLOR(set_color, flags, attr_color) \
    do { \
        base_color = (set_color); \
    } while(0)

// Main rendering function - matches rr_renderer_draw_mob signature
void rr_renderer_draw_hell_creek_mob(struct rr_renderer *renderer, uint8_t mob_id,
                                      float raw_animation_tick, float turning_value,
                                      uint8_t flags)
{
    // Reset any color filters that might have been applied
    rr_renderer_reset_color_filter(renderer);
    
    float animation_value = sinf(raw_animation_tick);
    float radius = RR_MOB_RARITY_SCALING[rr_rarity_id_common].radius * 20.0f; // Default radius, will be scaled by caller
    uint32_t seed = (uint32_t)(raw_animation_tick * 1000.0f); // Use animation as seed
    uint8_t attr_color = 0; // Default color
    uint32_t base_color = 0xffffe763;
    struct rr_renderer_context_state state;
    
    switch (mob_id)
    {
    case rr_mob_id_pachycephalosaurus: // BabyAnt
        SET_BASE_COLOR(0xff555555, flags, attr_color);
        rr_renderer_set_stroke(renderer, 0xff292929);
        rr_renderer_set_line_width(renderer, 7);
        rr_renderer_set_line_cap(renderer, 1); // round
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 0, -7);
        rr_renderer_quadratic_curve_to(renderer, 11, -10 + animation_value, 22, -5 + animation_value);
        rr_renderer_move_to(renderer, 0, 7);
        rr_renderer_quadratic_curve_to(renderer, 11, 10 - animation_value, 22, 5 - animation_value);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, 0, radius);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
        
    case rr_mob_id_triceratops: // WorkerAnt
        SET_BASE_COLOR(0xff555555, flags, attr_color);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 7);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, -12, 0, 10);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_set_stroke(renderer, 0xff292929);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 4, -7);
        rr_renderer_quadratic_curve_to(renderer, 15, -10 + animation_value, 26, -5 + animation_value);
        rr_renderer_move_to(renderer, 4, 7);
        rr_renderer_quadratic_curve_to(renderer, 15, 10 - animation_value, 26, 5 - animation_value);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 4, 0, radius);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
        
    case rr_mob_id_trex: // SoldierAnt
        SET_BASE_COLOR(0xff555555, flags, attr_color);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 7);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, -12, 0, 10);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, 0x80eeeeee);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_begin_path(renderer);
        rr_renderer_rotate(renderer, 0.1f * animation_value);
        rr_renderer_translate(renderer, -11, -8);
        rr_renderer_rotate(renderer, 0.1f * M_PI);
        rr_renderer_ellipse(renderer, 0, 0, 15, 7);
        rr_renderer_fill(renderer);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_begin_path(renderer);
        rr_renderer_rotate(renderer, -0.1f * animation_value);
        rr_renderer_translate(renderer, -11, 8);
        rr_renderer_rotate(renderer, -0.1f * M_PI);
        rr_renderer_ellipse(renderer, 0, 0, 15, 7);
        rr_renderer_fill(renderer);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_set_stroke(renderer, 0xff292929);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 4, -7);
        rr_renderer_quadratic_curve_to(renderer, 15, -10 + animation_value, 26, -5 + animation_value);
        rr_renderer_move_to(renderer, 4, 7);
        rr_renderer_quadratic_curve_to(renderer, 15, 10 - animation_value, 26, 5 - animation_value);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 4, 0, radius);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
        
    case rr_mob_id_ornithomimus: // Bee fern
        SET_BASE_COLOR(0xffffe763, flags, attr_color);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_set_stroke(renderer, 0xff292929);
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_set_line_join(renderer, 1);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, -25, 9);
        rr_renderer_line_to(renderer, -37, 0);
        rr_renderer_line_to(renderer, -25, -9);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_begin_path(renderer);
        rr_renderer_ellipse(renderer, 0, 0, 30, 20);
        rr_renderer_fill(renderer);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_clip(renderer);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_fill_rect(renderer, -30, -20, 10, 40);
        rr_renderer_fill_rect(renderer, -10, -20, 10, 40);
        rr_renderer_fill_rect(renderer, 10, -20, 10, 40);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_begin_path(renderer);
        rr_renderer_ellipse(renderer, 0, 0, 30, 20);
        rr_renderer_stroke(renderer);
        rr_renderer_set_stroke(renderer, 0xff333333);
        rr_renderer_set_line_width(renderer, 3);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 25, -5);
        rr_renderer_quadratic_curve_to(renderer, 35, -5, 40, -15);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 40, -15, 5);
        rr_renderer_fill(renderer);
        rr_renderer_set_stroke(renderer, 0xff333333);
        rr_renderer_set_line_width(renderer, 3);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 25, 5);
        rr_renderer_quadratic_curve_to(renderer, 35, 5, 40, 15);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 40, 15, 5);
        rr_renderer_fill(renderer);
        break;
        
    case rr_mob_id_tree: // Ladybug tree
        rr_renderer_scale(renderer, radius / 30.0f);
        SET_BASE_COLOR(0xffeb4034, flags, attr_color);
        rr_renderer_set_fill(renderer, 0xff111111);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 15, 0, 18.5f);
        rr_renderer_fill(renderer);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 24.76f, 16.94f);
        rr_renderer_quadratic_curve_to(renderer, 17.74f, 27.20f, 5.53f, 29.49f);
        rr_renderer_quadratic_curve_to(renderer, -6.68f, 31.78f, -16.94f, 24.76f);
        rr_renderer_quadratic_curve_to(renderer, -27.20f, 17.74f, -29.49f, 5.53f);
        rr_renderer_quadratic_curve_to(renderer, -31.78f, -6.68f, -24.76f, -16.94f);
        rr_renderer_quadratic_curve_to(renderer, -17.74f, -27.20f, -5.53f, -29.49f);
        rr_renderer_quadratic_curve_to(renderer, 6.68f, -31.78f, 16.94f, -24.76f);
        rr_renderer_quadratic_curve_to(renderer, 19.24f, -23.19f, 21.21f, -21.21f);
        rr_renderer_quadratic_curve_to(renderer, 23.19f, -19.24f, 24.76f, -16.94f);
        rr_renderer_quadratic_curve_to(renderer, 10, 0, 24.76f, 16.94f);
        rr_renderer_fill(renderer);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_clip(renderer);
        if (mob_id == rr_mob_id_tree)
            rr_renderer_set_fill(renderer, 0xff111111);
        else
            rr_renderer_set_fill(renderer, hsv_to_rgb(base_color, 1.2f));
        struct seed_gen gen;
        gen.seed = seed * 374572 + 46237;
        uint32_t ct = 1 + (uint32_t)(seed_next(&gen) * 7);
        for (uint32_t i = 0; i < ct; ++i)
        {
            rr_renderer_begin_path(renderer);
            rr_renderer_arc(renderer, seed_binext(&gen) * 30, seed_binext(&gen) * 30, 4 + seed_next(&gen) * 5);
            rr_renderer_fill(renderer);
        }
        rr_renderer_context_state_free(renderer, &state);
        // Additional ladybug wing details
        rr_renderer_set_fill(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 27.65f, 18.92f);
        rr_renderer_quadratic_curve_to(renderer, 19.81f, 30.37f, 6.18f, 32.93f);
        rr_renderer_quadratic_curve_to(renderer, -7.46f, 35.48f, -18.92f, 27.65f);
        rr_renderer_quadratic_curve_to(renderer, -30.37f, 19.81f, -32.93f, 6.18f);
        rr_renderer_quadratic_curve_to(renderer, -35.48f, -7.46f, -27.65f, -18.92f);
        rr_renderer_quadratic_curve_to(renderer, -19.81f, -30.37f, -6.18f, -32.93f);
        rr_renderer_quadratic_curve_to(renderer, 7.46f, -35.48f, 18.92f, -27.65f);
        rr_renderer_quadratic_curve_to(renderer, 24.10f, -24.10f, 27.65f, -18.92f);
        rr_renderer_quadratic_curve_to(renderer, 28.32f, -17.93f, 28.25f, -16.74f);
        rr_renderer_quadratic_curve_to(renderer, 28.18f, -15.54f, 27.40f, -14.64f);
        rr_renderer_quadratic_curve_to(renderer, 14.64f, 0, 27.40f, 14.64f);
        rr_renderer_quadratic_curve_to(renderer, 28.18f, 15.54f, 28.25f, 16.74f);
        rr_renderer_quadratic_curve_to(renderer, 28.32f, 17.93f, 27.65f, 18.92f);
        rr_renderer_line_to(renderer, 27.65f, 18.92f);
        rr_renderer_move_to(renderer, 21.87f, 14.96f);
        rr_renderer_line_to(renderer, 24.76f, 16.94f);
        rr_renderer_line_to(renderer, 22.12f, 19.24f);
        rr_renderer_quadratic_curve_to(renderer, 5.36f, 0, 22.12f, -19.24f);
        rr_renderer_line_to(renderer, 24.76f, -16.94f);
        rr_renderer_line_to(renderer, 21.87f, -14.96f);
        rr_renderer_quadratic_curve_to(renderer, 19.07f, -19.07f, 14.96f, -21.87f);
        rr_renderer_quadratic_curve_to(renderer, 5.90f, -28.07f, -4.88f, -26.05f);
        rr_renderer_quadratic_curve_to(renderer, -15.67f, -24.02f, -21.87f, -14.96f);
        rr_renderer_quadratic_curve_to(renderer, -28.07f, -5.90f, -26.05f, 4.88f);
        rr_renderer_quadratic_curve_to(renderer, -24.02f, 15.67f, -14.96f, 21.87f);
        rr_renderer_quadratic_curve_to(renderer, -5.90f, 28.07f, 4.88f, 26.05f);
        rr_renderer_quadratic_curve_to(renderer, 15.67f, 24.02f, 21.87f, 14.96f);
        rr_renderer_fill(renderer);
        break;
        
    case rr_mob_id_ankylosaurus: // Beetle
        rr_renderer_scale(renderer, radius / 35.0f);
        SET_BASE_COLOR(0xff905db0, flags, attr_color);
        rr_renderer_begin_path(renderer);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_set_stroke(renderer, 0xff333333);
        rr_renderer_set_line_width(renderer, 7);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_set_line_join(renderer, 1);
        rr_renderer_translate(renderer, 35, 0);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_rotate(renderer, -0.1f * animation_value);
        rr_renderer_move_to(renderer, -10, 15);
        rr_renderer_quadratic_curve_to(renderer, 15, 30, 35, 15);
        rr_renderer_quadratic_curve_to(renderer, 15, 20, -10, 5);
        rr_renderer_line_to(renderer, -10, 15);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_rotate(renderer, 0.1f * animation_value);
        rr_renderer_move_to(renderer, -10, -15);
        rr_renderer_quadratic_curve_to(renderer, 15, -30, 35, -15);
        rr_renderer_quadratic_curve_to(renderer, 15, -20, -10, -5);
        rr_renderer_line_to(renderer, -10, -15);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_translate(renderer, -35, 0);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 0, -30);
        rr_renderer_quadratic_curve_to(renderer, 40, -30, 40, 0);
        rr_renderer_quadratic_curve_to(renderer, 40, 30, 0, 30);
        rr_renderer_quadratic_curve_to(renderer, -40, 30, -40, 0);
        rr_renderer_quadratic_curve_to(renderer, -40, -30, 0, -30);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_fill(renderer);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 0, -33.5f);
        rr_renderer_quadratic_curve_to(renderer, 43.5f, -33.5f, 43.5f, 0);
        rr_renderer_quadratic_curve_to(renderer, 43.5f, 33.5f, 0, 33.5f);
        rr_renderer_quadratic_curve_to(renderer, -43.5f, 33.5f, -43.5f, 0);
        rr_renderer_quadratic_curve_to(renderer, -43.5f, -33.5f, 0, -33.5f);
        rr_renderer_move_to(renderer, 0, -26.5f);
        rr_renderer_quadratic_curve_to(renderer, -36.5f, -26.5f, -36.5f, 0);
        rr_renderer_quadratic_curve_to(renderer, -36.5f, 26.5f, 0, 26.5f);
        rr_renderer_quadratic_curve_to(renderer, 36.5f, 26.5f, 36.5f, 0);
        rr_renderer_quadratic_curve_to(renderer, 36.5f, -26.5f, 0, -26.5f);
        rr_renderer_set_fill(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_fill(renderer);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, -20, 0);
        rr_renderer_quadratic_curve_to(renderer, 0, -3, 20, 0);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 7);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, -17, -12);
        rr_renderer_arc(renderer, -17, -12, 5);
        rr_renderer_move_to(renderer, -17, -2);
        rr_renderer_arc(renderer, -17, 12, 5);
        rr_renderer_move_to(renderer, 0, -15);
        rr_renderer_arc(renderer, 0, -15, 5);
        rr_renderer_move_to(renderer, 0, 15);
        rr_renderer_arc(renderer, 0, 15, 5);
        rr_renderer_move_to(renderer, 17, -12);
        rr_renderer_arc(renderer, 17, -12, 5);
        rr_renderer_move_to(renderer, 17, 12);
        rr_renderer_arc(renderer, 17, 12, 5);
        rr_renderer_fill(renderer);
        break;
        
    case rr_mob_id_pteranodon: // Hornet
        SET_BASE_COLOR(0xffffe763, flags, attr_color);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_set_stroke(renderer, 0xff292929);
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_set_line_join(renderer, 1);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, -25, -6);
        rr_renderer_line_to(renderer, -47, 0);
        rr_renderer_line_to(renderer, -25, 6);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_begin_path(renderer);
        rr_renderer_ellipse(renderer, 0, 0, 30, 20);
        rr_renderer_fill(renderer);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_clip(renderer);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_fill_rect(renderer, -30, -20, 10, 40);
        rr_renderer_fill_rect(renderer, -10, -20, 10, 40);
        rr_renderer_fill_rect(renderer, 10, -20, 10, 40);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_begin_path(renderer);
        rr_renderer_ellipse(renderer, 0, 0, 30, 20);
        rr_renderer_stroke(renderer);
        rr_renderer_set_stroke(renderer, 0xff333333);
        rr_renderer_set_line_width(renderer, 3);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, 25, 5);
        rr_renderer_quadratic_curve_to(renderer, 40, 10, 50, 15);
        rr_renderer_quadratic_curve_to(renderer, 40, 5, 25, 5);
        rr_renderer_move_to(renderer, 25, -5);
        rr_renderer_quadratic_curve_to(renderer, 40, -10, 50, -15);
        rr_renderer_quadratic_curve_to(renderer, 40, -5, 25, -5);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
        
    case rr_mob_id_quetzalcoatlus: // Cactus dakotaraptor
    {
        SET_BASE_COLOR(0xff32a852, flags, attr_color);
        uint32_t vertices = (uint32_t)(radius / 10.0f + 5);
        rr_renderer_context_state_init(renderer, &state);
        rr_renderer_set_fill(renderer, 0xff222222);
        rr_renderer_begin_path(renderer);
        for (uint32_t i = 0; i < vertices; ++i)
        {
            float angle = 2.0f * M_PI * i / vertices;
            rr_renderer_move_to(renderer, 10 + radius, 0);
            rr_renderer_line_to(renderer, 0.5f + radius, 3);
            rr_renderer_line_to(renderer, 0.5f + radius, -3);
            rr_renderer_line_to(renderer, 10 + radius, 0);
            rr_renderer_rotate(renderer, M_PI * 2.0f / vertices);
        }
        rr_renderer_fill(renderer);
        rr_renderer_context_state_free(renderer, &state);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_set_line_join(renderer, 1);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, radius, 0);
        for (uint32_t i = 0; i < vertices; ++i)
        {
            float base_angle = M_PI * 2.0f * i / vertices;
            rr_renderer_quadratic_curve_to(renderer,
                radius * 0.9f * cosf(base_angle + M_PI / vertices),
                radius * 0.9f * sinf(base_angle + M_PI / vertices),
                radius * cosf(base_angle + 2.0f * M_PI / vertices),
                radius * sinf(base_angle + 2.0f * M_PI / vertices));
        }
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
    }
    
    case rr_mob_id_fern: // Rock ornithomimus
    {
        SET_BASE_COLOR(0xff777777, flags, attr_color);
        struct seed_gen gen;
        gen.seed = (uint32_t)floorf(radius) * 1957264 + 295726;
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_set_line_join(renderer, 1);
        rr_renderer_begin_path(renderer);
        float deflection = radius * 0.1f;
        rr_renderer_move_to(renderer, radius + seed_binext(&gen) * deflection, seed_binext(&gen) * deflection);
        uint32_t sides = 4 + (uint32_t)(radius / 10.0f);
        for (uint32_t i = 1; i < sides; ++i)
        {
            float angle = 2.0f * M_PI * i / sides;
            rr_renderer_line_to(renderer,
                cosf(angle) * radius + seed_binext(&gen) * deflection,
                sinf(angle) * radius + seed_binext(&gen) * deflection);
        }
        // Close path by returning to start
        rr_renderer_line_to(renderer, radius + seed_binext(&gen) * deflection, seed_binext(&gen) * deflection);
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
    }
    
    case rr_mob_id_meteor: // Centipede
        if (mob_id == rr_mob_id_meteor)
            SET_BASE_COLOR(0xff8ac255, flags, attr_color);
        else
            SET_BASE_COLOR(0xff905db0, flags, attr_color);
        rr_renderer_set_fill(renderer, 0xff333333);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, -30, 15);
        rr_renderer_fill(renderer);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, 30, 15);
        rr_renderer_fill(renderer);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, 0, 35);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_fill(renderer);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 7);
        rr_renderer_stroke(renderer);
        if (!(flags & 2))
        {
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 25, -10);
            rr_renderer_quadratic_curve_to(renderer, 45, -10, 55, -30);
            rr_renderer_set_stroke(renderer, 0xff333333);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_stroke(renderer);
            rr_renderer_begin_path(renderer);
            rr_renderer_arc(renderer, 55, -30, 5);
            rr_renderer_set_fill(renderer, 0xff333333);
            rr_renderer_fill(renderer);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 25, 10);
            rr_renderer_quadratic_curve_to(renderer, 45, 10, 55, 30);
            rr_renderer_stroke(renderer);
            rr_renderer_begin_path(renderer);
            rr_renderer_arc(renderer, 55, 30, 5);
            rr_renderer_fill(renderer);
        }
        break;
        
    case rr_mob_id_dakotaraptor: // Spider quetzalcoatlus
        SET_BASE_COLOR(0xff4f412e, flags, attr_color);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, 0xff333333);
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_begin_path(renderer);
        float anim_sin = sinf(animation_value);
        float anim_cos = cosf(animation_value);
        // Leg 1
        {
            float angle = -M_PI + 0.9f + anim_sin * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 2
        {
            float angle = -M_PI + 0.3f + anim_cos * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 3
        {
            float angle = -M_PI - 0.3f + anim_sin * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 4
        {
            float angle = -M_PI - 0.9f - anim_cos * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 5
        {
            float angle = -0.9f - anim_sin * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 6
        {
            float angle = -0.3f + anim_cos * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 7
        {
            float angle = 0.3f - anim_sin * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        // Leg 8
        {
            float angle = 0.9f - anim_cos * 0.2f;
            float cos_val = cosf(angle) * 35;
            float sin_val = sinf(angle) * 35;
            rr_renderer_move_to(renderer, 0, 0);
            rr_renderer_quadratic_curve_to(renderer, sin_val * 0.8f, cos_val * 0.5f, sin_val, cos_val);
        }
        rr_renderer_stroke(renderer);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, 0, radius);
        rr_renderer_fill(renderer);
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_line_width(renderer, 5);
        rr_renderer_stroke(renderer);
        break;
        
    case rr_mob_id_edmontosaurus: // Sandstorm
        SET_BASE_COLOR(0xffd5c7a6, flags, attr_color);
        rr_renderer_set_line_width(renderer, radius / 5.0f);
        rr_renderer_set_line_cap(renderer, 1);
        rr_renderer_set_line_join(renderer, 1);
        rr_renderer_set_fill(renderer, base_color);
        rr_renderer_set_stroke(renderer, base_color);
        rr_renderer_rotate(renderer, animation_value / 3.0f);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, radius, 0);
        for (uint32_t i = 1; i <= 6; ++i)
        {
            float angle = 2.0f * M_PI * i / 6.0f;
            rr_renderer_line_to(renderer, cosf(angle) * radius, sinf(angle) * radius);
        }
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, hsv_to_rgb(base_color, 0.9f));
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.9f));
        rr_renderer_rotate(renderer, animation_value / 3.0f);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, radius * 2.0f / 3.0f, 0);
        for (uint32_t i = 1; i <= 6; ++i)
        {
            float angle = 2.0f * M_PI * i / 6.0f;
            rr_renderer_line_to(renderer, cosf(angle) * radius * 2.0f / 3.0f, sinf(angle) * radius * 2.0f / 3.0f);
        }
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        rr_renderer_set_fill(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_set_stroke(renderer, hsv_to_rgb(base_color, 0.8f));
        rr_renderer_rotate(renderer, animation_value / 3.0f);
        rr_renderer_begin_path(renderer);
        rr_renderer_move_to(renderer, radius / 3.0f, 0);
        for (uint32_t i = 1; i <= 6; ++i)
        {
            float angle = 2.0f * M_PI * i / 6.0f;
            rr_renderer_line_to(renderer, cosf(angle) * radius / 3.0f, sinf(angle) * radius / 3.0f);
        }
        rr_renderer_fill(renderer);
        rr_renderer_stroke(renderer);
        break;
        
    default:
        // Fallback rendering
        rr_renderer_set_fill(renderer, 0xffff0000);
        rr_renderer_begin_path(renderer);
        rr_renderer_arc(renderer, 0, 0, radius);
        rr_renderer_fill(renderer);
        break;
    }
}

