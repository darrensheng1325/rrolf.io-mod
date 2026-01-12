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

#include <Client/Assets/RenderFunctions.h>

#include <math.h>
#include <stdint.h>

#include <Client/Renderer/Renderer.h>
#include <Shared/StaticData.h>

#define IMAGE_SIZE (256.0f)
struct rr_renderer petal_cache;

// Helper functions for petal rendering
static uint32_t hsv_to_rgb(uint32_t color, float saturation_mult)
{
    uint8_t r = (color >> 16) & 0xff;
    uint8_t g = (color >> 8) & 0xff;
    uint8_t b = color & 0xff;
    
    float max = fmaxf(fmaxf(r, g), b) / 255.0f;
    float min = fminf(fminf(r, g), b) / 255.0f;
    float delta = max - min;
    
    if (delta < 0.001f)
        return color;
    
    float s = delta / max;
    s *= saturation_mult;
    s = fminf(1.0f, fmaxf(0.0f, s));
    
    float new_delta = max * s;
    float m = max - new_delta;
    
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
    
    float new_r = 0, new_g = 0, new_b = 0;
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

void rr_renderer_draw_petal(struct rr_renderer *renderer, uint8_t id,
                            uint8_t flags)
{
    if (flags & 1)
    {
        rr_renderer_scale(renderer, 50 / IMAGE_SIZE);
        rr_renderer_draw_clipped_image(
            renderer, &petal_cache, IMAGE_SIZE / 2 + IMAGE_SIZE * id,
            IMAGE_SIZE / 2, IMAGE_SIZE, IMAGE_SIZE, 0, 0);
        rr_renderer_scale(renderer, IMAGE_SIZE / 50);
    }
    else
    {
        switch (id)
        {
        case rr_petal_id_none:
            break;
        case rr_petal_id_basic: // kBasic, kUniqueBasic, kTwin, kTriplet
            rr_renderer_set_fill(renderer, 0xffffffff);
            rr_renderer_set_stroke(renderer, 0xffcfcfcf);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
            rr_renderer_arc(renderer, 0, 0, 10.0f);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            break;
        case rr_petal_id_light: // kFaster
            {
                float r = 10.0f; // Default radius
                rr_renderer_set_fill(renderer, 0xfffeffc9);
                rr_renderer_set_stroke(renderer, 0xffcecfa3);
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_begin_path(renderer);
                rr_renderer_arc(renderer, 0, 0, r);
                rr_renderer_fill(renderer);
                rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_pellet: // kTwin, kTriplet
            rr_renderer_set_fill(renderer, 0xffffffff);
            rr_renderer_set_stroke(renderer, 0xffcfcfcf);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
            rr_renderer_arc(renderer, 0, 0, 10.0f);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            break;
        case rr_petal_id_stinger: // kStinger, kTringer
            {
                float r = 10.0f; // Default radius
            rr_renderer_set_fill(renderer, 0xff333333);
            rr_renderer_set_stroke(renderer, 0xff292929);
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_set_line_cap(renderer, 1);
                rr_renderer_set_line_join(renderer, 1);
            rr_renderer_begin_path(renderer);
                rr_renderer_move_to(renderer, r, 0);
                rr_renderer_line_to(renderer, -r * 0.5f, r * 0.866f);
                rr_renderer_line_to(renderer, -r * 0.5f, -r * 0.866f);
                rr_renderer_line_to(renderer, r, 0);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_fossil: // kRock
            {
                float r = 15.0f; // Default radius
                rr_renderer_set_fill(renderer, 0xff777777);
                rr_renderer_set_stroke(renderer, hsv_to_rgb(0xff777777, 0.8f));
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_set_line_cap(renderer, 1);
                rr_renderer_set_line_join(renderer, 1);
            rr_renderer_begin_path(renderer);
                rr_renderer_move_to(renderer, 12.138f, 0);
                rr_renderer_line_to(renderer, 3.841f, 12.377f);
                rr_renderer_line_to(renderer, -11.312f, 7.917f);
                rr_renderer_line_to(renderer, -11.461f, -7.837f);
                rr_renderer_line_to(renderer, 4.538f, -13.892f);
                rr_renderer_line_to(renderer, 12.138f, 0);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_shell:
            rr_renderer_scale(renderer, 0.15f);
            rr_renderer_set_fill(renderer, 0xffd2c6a7);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -64.46, -11.54);
            rr_renderer_line_to(renderer, -32.45, 17.66);
            rr_renderer_line_to(renderer, -14.48, 58.65);
            rr_renderer_line_to(renderer, 32.69, 57.52);
            rr_renderer_line_to(renderer, 64.14, 14.29);
            rr_renderer_line_to(renderer, 52.35, -39.05);
            rr_renderer_line_to(renderer, 5.73, -62.64);
            rr_renderer_line_to(renderer, -40.88, -49.72);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 14.67, -18.12);
            rr_renderer_bezier_curve_to(renderer, 28.33, -14.69, 37.91, -2.41,
                                        37.91, 11.68);
            rr_renderer_line_to(renderer, 22.55, 11.68);
            rr_renderer_bezier_curve_to(renderer, 22.55, 4.63, 17.76, -1.51,
                                        10.93, -3.22);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 37.91, 11.68);
            rr_renderer_bezier_curve_to(renderer, 37.91, 24.99, 28.12, 36.28,
                                        14.94, 38.17);
            rr_renderer_line_to(renderer, 12.75, 22.89);
            rr_renderer_bezier_curve_to(renderer, 18.34, 22.09, 22.48, 17.31,
                                        22.48, 11.68);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -17.49, -11.18);
            rr_renderer_bezier_curve_to(renderer, -8.31, -18.25, 3.57, -20.80,
                                        14.84, -18.10);
            rr_renderer_line_to(renderer, 11.25, -3.09);
            rr_renderer_bezier_curve_to(renderer, 4.51, -4.70, -2.58, -3.18,
                                        -8.07, 1.05);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -36.75, 19.23);
            rr_renderer_bezier_curve_to(renderer, -33.84, 7.06, -26.87, -3.76,
                                        -16.98, -11.44);
            rr_renderer_line_to(renderer, -7.52, 0.74);
            rr_renderer_bezier_curve_to(renderer, -14.64, 6.27, -19.66, 14.05,
                                        -21.75, 22.81);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 5.91, 30.51);
            rr_renderer_bezier_curve_to(renderer, 5.91, 26.29, 9.33, 22.87,
                                        13.55, 22.87);
            rr_renderer_bezier_curve_to(renderer, 15.58, 22.87, 17.53, 23.67,
                                        18.96, 25.10);
            rr_renderer_bezier_curve_to(renderer, 20.39, 26.54, 21.20, 28.48,
                                        21.20, 30.51);
            rr_renderer_bezier_curve_to(renderer, 21.20, 34.73, 17.78, 38.15,
                                        13.55, 38.15);
            rr_renderer_bezier_curve_to(renderer, 9.33, 38.15, 5.91, 34.73,
                                        5.91, 30.51);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -21.54, 59.49);
            rr_renderer_bezier_curve_to(renderer, -34.43, 50.38, -40.41, 34.30,
                                        -36.62, 18.99);
            rr_renderer_line_to(renderer, -21.63, 22.70);
            rr_renderer_bezier_curve_to(renderer, -23.90, 31.84, -20.32, 41.45,
                                        -12.63, 46.89);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 41.64, 59.40);
            rr_renderer_bezier_curve_to(renderer, 22.66, 72.71, -2.62, 72.72,
                                        -21.60, 59.40);
            rr_renderer_line_to(renderer, -12.74, 46.78);
            rr_renderer_bezier_curve_to(renderer, 0.92, 56.36, 19.12, 56.36,
                                        32.78, 46.78);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -70.55, -16.55);
            rr_renderer_bezier_curve_to(renderer, -62.55, -47.21, -35.37,
                                        -69.00, -3.70, -70.14);
            rr_renderer_bezier_curve_to(renderer, 27.98, -71.29, 56.66, -51.52,
                                        66.86, -21.52);
            rr_renderer_bezier_curve_to(renderer, 77.06, 8.49, 66.36, 41.63,
                                        40.55, 60.02);
            rr_renderer_line_to(renderer, 31.58, 47.44);
            rr_renderer_bezier_curve_to(renderer, 51.84, 33.01, 60.23, 7.00,
                                        52.23, -16.54);
            rr_renderer_bezier_curve_to(renderer, 44.22, -40.09, 21.72, -55.60,
                                        -3.14, -54.70);
            rr_renderer_bezier_curve_to(renderer, -27.99, -53.80, -49.32,
                                        -36.71, -55.60, -12.65);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -29.38, 23.46);
            rr_renderer_bezier_curve_to(renderer, -49.26, 23.42, -66.48, 9.64,
                                        -70.86, -9.75);
            rr_renderer_line_to(renderer, -55.47, -13.22);
            rr_renderer_bezier_curve_to(renderer, -52.72, -1.01, -41.88, 7.67,
                                        -29.35, 7.69);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffaa9e80);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -71.53, -13.09);
            rr_renderer_bezier_curve_to(renderer, -71.53, -17.31, -68.11,
                                        -20.73, -63.89, -20.73);
            rr_renderer_bezier_curve_to(renderer, -61.86, -20.73, -59.91,
                                        -19.93, -58.48, -18.49);
            rr_renderer_bezier_curve_to(renderer, -57.05, -17.06, -56.24,
                                        -15.12, -56.24, -13.09);
            rr_renderer_bezier_curve_to(renderer, -56.24, -8.87, -59.66, -5.45,
                                        -63.89, -5.45);
            rr_renderer_bezier_curve_to(renderer, -68.11, -5.45, -71.53, -8.87,
                                        -71.53, -13.09);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_peas: // kPeas
            {
                float r = 13.0f; // Default radius for peas
            rr_renderer_set_fill(renderer, 0xff8ac255);
                rr_renderer_set_stroke(renderer, 0xff709d45);
                rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
                rr_renderer_arc(renderer, 0, 0, r);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_leaf: // kLeaf, kGoldenLeaf
            {
                float r = 15.0f; // Default radius
                // Use green for regular leaf, yellow for golden leaf
                uint32_t fill_color = 0xff39b54a; // kLeaf
                uint32_t stroke_color = 0xff2e933c;
                // Note: kGoldenLeaf would use 0xffebeb34 / 0xffbebe2a, but we only have one leaf type
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_set_line_cap(renderer, 1);
                rr_renderer_set_line_join(renderer, 1);
                rr_renderer_set_fill(renderer, fill_color);
                rr_renderer_set_stroke(renderer, stroke_color);
            rr_renderer_begin_path(renderer);
                rr_renderer_move_to(renderer, -20, 0);
                rr_renderer_bezier_curve_to(renderer, -10, -12, 5, -12, 15, 0);
                rr_renderer_bezier_curve_to(renderer, 5, 12, -10, 12, -15, 0);
            rr_renderer_fill(renderer);
                rr_renderer_stroke(renderer);
            rr_renderer_begin_path(renderer);
                rr_renderer_move_to(renderer, -9, 0);
                rr_renderer_quadratic_curve_to(renderer, 0, -1.5f, 7.5f, 0);
                rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_egg: // kAntEgg
            {
                float r = 12.0f; // Default radius
                rr_renderer_set_stroke(renderer, 0xffcfc295);
                rr_renderer_set_fill(renderer, 0xfffff0b8);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
                rr_renderer_arc(renderer, 0, 0, r);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_magnet:
            rr_renderer_scale(renderer, 0.5);
            rr_renderer_translate(renderer, -20.00, 0.00);
            rr_renderer_set_line_cap(renderer, 1);
            rr_renderer_set_stroke(renderer, 0xff363685);
            rr_renderer_set_line_width(renderer, 28);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 39.50, 18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, 30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_set_stroke(renderer, 0xff4343a4);
            rr_renderer_set_line_width(renderer, 16.8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 40.00, 18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, 30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_set_stroke(renderer, 0xff853636);
            rr_renderer_set_line_width(renderer, 28);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 39.50, -18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, -30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_set_stroke(renderer, 0xffa44343);
            rr_renderer_set_line_width(renderer, 16.8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 40.00, -18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, -30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_begin_path(renderer);
            rr_renderer_set_stroke(renderer, 0xff363685);
            rr_renderer_set_line_width(renderer, 28);
            rr_renderer_move_to(renderer, 39.50, 18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, 30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_set_stroke(renderer, 0xff4343a4);
            rr_renderer_set_line_width(renderer, 16.8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 40.00, 18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, 30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_set_line_cap(renderer, 0);
            rr_renderer_set_stroke(renderer, 0xff853636);
            rr_renderer_set_line_width(renderer, 28);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 39.50, -18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, -30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            rr_renderer_set_stroke(renderer, 0xffa44343);
            rr_renderer_set_line_width(renderer, 16.8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 40.00, -18.00);
            rr_renderer_quadratic_curve_to(renderer, 0.00, -30.00, 0.00, 0.00);
            rr_renderer_stroke(renderer);
            break;
        case rr_petal_id_uranium:
            rr_renderer_set_fill(renderer, 0xff63bf2e);
            rr_renderer_set_stroke(renderer, 0xff509b25);
            rr_renderer_set_line_cap(renderer, 1);
            rr_renderer_set_line_join(renderer, 1);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -7.00, -5.00);
            rr_renderer_line_to(renderer, -1.00, -9.00);
            rr_renderer_line_to(renderer, 7.00, -6.00);
            rr_renderer_line_to(renderer, 9.00, 3.00);
            rr_renderer_line_to(renderer, 2.00, 9.00);
            rr_renderer_line_to(renderer, -5.00, 6.00);
            rr_renderer_line_to(renderer, -8.00, 2.00);
            rr_renderer_line_to(renderer, -7.00, -5.00);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            break;
        case rr_petal_id_feather: // kSalt
            {
                rr_renderer_set_fill(renderer, 0xffffffff);
                rr_renderer_set_stroke(renderer, 0xffcfcfcf);
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_set_line_cap(renderer, 1);
                rr_renderer_set_line_join(renderer, 1);
                rr_renderer_begin_path(renderer);
                rr_renderer_move_to(renderer, 10.404f, 0);
                rr_renderer_line_to(renderer, 6.643f, 8.722f);
                rr_renderer_line_to(renderer, -2.667f, 11.255f);
                rr_renderer_line_to(renderer, -10.940f, 4.958f);
                rr_renderer_line_to(renderer, -11.342f, -5.432f);
                rr_renderer_line_to(renderer, -2.497f, -11.472f);
                rr_renderer_line_to(renderer, 7.798f, -9.585f);
                rr_renderer_line_to(renderer, 10.404f, 0);
                rr_renderer_fill(renderer);
                rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_azalea: // kRose, kDahlia
            {
                float r = 10.0f; // Default radius
                rr_renderer_set_fill(renderer, 0xffff94c9);
                rr_renderer_set_stroke(renderer, 0xffcf78a3);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
                rr_renderer_arc(renderer, 0, 0, r);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_bone: // kBone
            {
                float r = 12.0f; // Default radius
                rr_renderer_set_fill(renderer, 0xffffffff);
                rr_renderer_set_stroke(renderer, 0xffcfcfcf);
                rr_renderer_set_line_width(renderer, 5);
                rr_renderer_scale(renderer, r / 12.0f);
            rr_renderer_begin_path(renderer);
                rr_renderer_move_to(renderer, -10, -4);
                rr_renderer_quadratic_curve_to(renderer, 0, 0, 10, -4);
                rr_renderer_bezier_curve_to(renderer, 14, -10, 20, -2, 14, 0);
                rr_renderer_bezier_curve_to(renderer, 20, 2, 14, 10, 10, 4);
                rr_renderer_quadratic_curve_to(renderer, 0, 0, -10, 4);
                rr_renderer_bezier_curve_to(renderer, -14, 10, -20, 2, -14, 0);
                rr_renderer_bezier_curve_to(renderer, -20, -2, -14, -10, -10, -4);
                rr_renderer_stroke(renderer);
            rr_renderer_fill(renderer);
            }
            break;
        case rr_petal_id_web:
            rr_renderer_set_fill(renderer, 0xffffffff);
            rr_renderer_set_stroke(renderer, 0xffcfcfcf);
            rr_renderer_set_line_cap(renderer, 1);
            rr_renderer_set_line_join(renderer, 1);
            rr_renderer_set_line_width(renderer, 3);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 11.00, 0.00);
            rr_renderer_quadratic_curve_to(renderer, 4.32, 3.14, 3.40, 10.46);
            rr_renderer_quadratic_curve_to(renderer, -1.65, 5.08, -8.90, 6.47);
            rr_renderer_quadratic_curve_to(renderer, -5.34, -0.00, -8.90, -6.47);
            rr_renderer_quadratic_curve_to(renderer, -1.65, -5.08, 3.40, -10.46);
            rr_renderer_quadratic_curve_to(renderer, 4.32, -3.14, 11.00, 0.00);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            break;
        case rr_petal_id_seed:
            rr_renderer_scale(renderer, 0.1);
            rr_renderer_set_fill(renderer, 0xffb07200);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 80.90, -82.33);
            rr_renderer_bezier_curve_to(renderer, 125.72, -37.51, 125.72, 35.15,
                                        80.90, 79.97);
            rr_renderer_bezier_curve_to(renderer, 59.38, 101.49, 30.19, 113.58,
                                        -0.25, 113.58);
            rr_renderer_bezier_curve_to(renderer, -30.68, 113.58, -59.87,
                                        101.49, -81.40, 79.97);
            rr_renderer_bezier_curve_to(renderer, -126.21, 35.15, -126.21,
                                        -37.51, -81.40, -82.33);
            rr_renderer_bezier_curve_to(renderer, -36.58, -127.15, 36.08,
                                        -127.15, 80.90, -82.33);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffb07200);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 95.58, 39.57);
            rr_renderer_bezier_curve_to(renderer, 110.79, 54.78, 110.79, 79.44,
                                        95.58, 94.65);
            rr_renderer_bezier_curve_to(renderer, 88.27, 101.95, 78.37, 106.05,
                                        68.04, 106.05);
            rr_renderer_bezier_curve_to(renderer, 57.71, 106.05, 47.80, 101.95,
                                        40.50, 94.65);
            rr_renderer_bezier_curve_to(renderer, 25.29, 79.44, 25.29, 54.78,
                                        40.50, 39.57);
            rr_renderer_bezier_curve_to(renderer, 55.71, 24.36, 80.37, 24.36,
                                        95.58, 39.57);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc98200);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -0.25, -82.33);
            rr_renderer_bezier_curve_to(renderer, 44.57, -82.33, 80.90, -46.00,
                                        80.90, -1.18);
            rr_renderer_quadratic_curve_to(renderer, 80.90, 34.64, 71.39,
                                           70.45);
            rr_renderer_quadratic_curve_to(renderer, 35.57, 79.97, -0.25,
                                           79.97);
            rr_renderer_bezier_curve_to(renderer, -45.07, 79.97, -81.40, 43.64,
                                        -81.40, -1.18);
            rr_renderer_bezier_curve_to(renderer, -81.40, -46.00, -45.07,
                                        -82.33, -0.25, -82.33);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffb07200);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -74.23, -77.81);
            rr_renderer_bezier_curve_to(renderer, -15.66, -81.51, 35.64, -38.94,
                                        42.81, 19.30);
            rr_renderer_line_to(renderer, 6.68, 23.75);
            rr_renderer_bezier_curve_to(renderer, 1.86, -15.37, -32.60, -43.96,
                                        -71.93, -41.48);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffb07200);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 37.65, 9.86);
            rr_renderer_bezier_curve_to(renderer, 44.74, 16.95, 44.74, 28.44,
                                        37.65, 35.53);
            rr_renderer_bezier_curve_to(renderer, 34.25, 38.93, 29.63, 40.84,
                                        24.82, 40.84);
            rr_renderer_bezier_curve_to(renderer, 20.00, 40.84, 15.39, 38.93,
                                        11.98, 35.53);
            rr_renderer_bezier_curve_to(renderer, 4.89, 28.44, 4.89, 16.95,
                                        11.98, 9.86);
            rr_renderer_bezier_curve_to(renderer, 19.07, 2.77, 30.56, 2.77,
                                        37.65, 9.86);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_gravel: // kPollen
            {
                float r = 10.0f; // Default radius
                rr_renderer_set_fill(renderer, 0xffffe763);
                rr_renderer_set_stroke(renderer, 0xffcfbb50);
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_begin_path(renderer);
                rr_renderer_arc(renderer, 0, 0, r);
                rr_renderer_fill(renderer);
                rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_club:
            rr_renderer_scale(renderer, 0.2);
            rr_renderer_set_fill(renderer, 0xff7d6c64);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 72.00, 2.38);
            rr_renderer_bezier_curve_to(renderer, 72.00, 31.37, 48.94, 54.87,
                                        20.50, 54.87);
            rr_renderer_bezier_curve_to(renderer, 6.84, 54.87, -6.26, 49.34,
                                        -15.92, 39.49);
            rr_renderer_bezier_curve_to(renderer, -25.58, 29.65, -31.00, 16.30,
                                        -31.00, 2.38);
            rr_renderer_bezier_curve_to(renderer, -31.00, -26.61, -7.94, -50.12,
                                        20.50, -50.12);
            rr_renderer_bezier_curve_to(renderer, 48.94, -50.12, 72.00, -26.61,
                                        72.00, 2.38);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff7d6c64);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 33.82, 37.00);
            rr_renderer_bezier_curve_to(renderer, 33.82, 56.04, 18.68, 71.48,
                                        0.00, 71.48);
            rr_renderer_bezier_curve_to(renderer, -8.97, 71.48, -17.57, 67.84,
                                        -23.92, 61.38);
            rr_renderer_bezier_curve_to(renderer, -30.26, 54.91, -33.82, 46.14,
                                        -33.82, 37.00);
            rr_renderer_bezier_curve_to(renderer, -33.82, 17.96, -18.68, 2.52,
                                        0.00, 2.52);
            rr_renderer_bezier_curve_to(renderer, 18.68, 2.52, 33.82, 17.96,
                                        33.82, 37.00);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff7d6c64);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 31.00, 2.38);
            rr_renderer_bezier_curve_to(renderer, 31.00, 31.37, 7.95, 54.87,
                                        -20.50, 54.87);
            rr_renderer_bezier_curve_to(renderer, -34.16, 54.87, -47.26, 49.34,
                                        -56.92, 39.49);
            rr_renderer_bezier_curve_to(renderer, -66.58, 29.65, -72.00, 16.30,
                                        -72.00, 2.38);
            rr_renderer_bezier_curve_to(renderer, -72.00, -26.61, -48.94,
                                        -50.12, -20.50, -50.12);
            rr_renderer_bezier_curve_to(renderer, 7.95, -50.12, 31.00, -26.61,
                                        31.00, 2.38);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff917d73);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 58.38, 3.81);
            rr_renderer_bezier_curve_to(renderer, 58.38, 25.13, 41.42, 42.41,
                                        20.50, 42.41);
            rr_renderer_bezier_curve_to(renderer, 10.45, 42.41, 0.82, 38.34,
                                        -6.28, 31.10);
            rr_renderer_bezier_curve_to(renderer, -13.39, 23.86, -17.38, 14.05,
                                        -17.38, 3.81);
            rr_renderer_bezier_curve_to(renderer, -17.38, -17.51, -0.42, -34.79,
                                        20.50, -34.79);
            rr_renderer_bezier_curve_to(renderer, 41.42, -34.79, 58.38, -17.51,
                                        58.38, 3.81);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff917d73);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 17.38, 3.81);
            rr_renderer_bezier_curve_to(renderer, 17.38, 25.13, 0.42, 42.41,
                                        -20.50, 42.41);
            rr_renderer_bezier_curve_to(renderer, -30.54, 42.41, -40.18, 38.34,
                                        -47.28, 31.10);
            rr_renderer_bezier_curve_to(renderer, -54.38, 23.86, -58.37, 14.05,
                                        -58.37, 3.81);
            rr_renderer_bezier_curve_to(renderer, -58.37, -17.51, -41.42,
                                        -34.79, -20.50, -34.79);
            rr_renderer_bezier_curve_to(renderer, 0.42, -34.79, 17.38, -17.51,
                                        17.38, 3.81);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff917d73);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 20.88, 37.00);
            rr_renderer_bezier_curve_to(renderer, 20.88, 48.75, 11.53, 58.28,
                                        0.00, 58.28);
            rr_renderer_bezier_curve_to(renderer, -5.54, 58.28, -10.85, 56.04,
                                        -14.76, 52.05);
            rr_renderer_bezier_curve_to(renderer, -18.68, 48.06, -20.88, 42.64,
                                        -20.88, 37.00);
            rr_renderer_bezier_curve_to(renderer, -20.88, 25.24, -11.53, 15.72,
                                        0.00, 15.72);
            rr_renderer_bezier_curve_to(renderer, 11.53, 15.72, 20.88, 25.24,
                                        20.88, 37.00);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff6a5045);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -22.82, -44.69);
            rr_renderer_line_to(renderer, -13.86, 4.25);
            rr_renderer_line_to(renderer, 1.65, 4.76);
            rr_renderer_line_to(renderer, -5.59, -38.66);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff6a5045);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 22.84, -44.69);
            rr_renderer_line_to(renderer, 13.88, 4.25);
            rr_renderer_line_to(renderer, -1.62, 4.76);
            rr_renderer_line_to(renderer, 5.61, -38.66);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff6a5045);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -14.10, 1.74);
            rr_renderer_bezier_curve_to(renderer, -14.10, -6.08, -7.78, -12.42,
                                        0.01, -12.42);
            rr_renderer_bezier_curve_to(renderer, 3.75, -12.42, 7.34, -10.92,
                                        9.99, -8.27);
            rr_renderer_bezier_curve_to(renderer, 12.64, -5.62, 14.12, -2.02,
                                        14.12, 1.74);
            rr_renderer_bezier_curve_to(renderer, 14.12, 9.55, 7.81, 15.89,
                                        0.01, 15.89);
            rr_renderer_bezier_curve_to(renderer, -7.78, 15.89, -14.10, 9.55,
                                        -14.10, 1.74);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff6a5045);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -23.10, -48.31);
            rr_renderer_bezier_curve_to(renderer, -23.10, -61.10, -12.75,
                                        -71.48, 0.01, -71.48);
            rr_renderer_bezier_curve_to(renderer, 6.14, -71.48, 12.02, -69.04,
                                        16.36, -64.69);
            rr_renderer_bezier_curve_to(renderer, 20.69, -60.34, 23.13, -54.45,
                                        23.13, -48.31);
            rr_renderer_bezier_curve_to(renderer, 23.13, -35.51, 12.78, -25.13,
                                        0.01, -25.13);
            rr_renderer_bezier_curve_to(renderer, -12.75, -25.13, -23.10,
                                        -35.51, -23.10, -48.31);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff856456);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 9.73, -47.69);
            rr_renderer_line_to(renderer, 0.01, 2.01);
            rr_renderer_line_to(renderer, -9.71, -47.69);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff856456);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -9.71, -48.30);
            rr_renderer_bezier_curve_to(renderer, -9.71, -53.68, -5.36, -58.04,
                                        0.01, -58.04);
            rr_renderer_bezier_curve_to(renderer, 2.59, -58.04, 5.06, -57.02,
                                        6.88, -55.19);
            rr_renderer_bezier_curve_to(renderer, 8.71, -53.36, 9.73, -50.89,
                                        9.73, -48.30);
            rr_renderer_bezier_curve_to(renderer, 9.73, -42.92, 5.38, -38.56,
                                        0.01, -38.56);
            rr_renderer_bezier_curve_to(renderer, -5.36, -38.56, -9.71, -42.92,
                                        -9.71, -48.30);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_crest:
            rr_renderer_scale(renderer, 0.25);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -71.19, -49.99);
            rr_renderer_bezier_curve_to(renderer, -61.70, -54.48, -41.04,
                                        -34.08, -25.06, -4.44);
            rr_renderer_bezier_curve_to(renderer, -9.07, 25.20, -3.81, 52.86,
                                        -13.30, 57.34);
            rr_renderer_bezier_curve_to(renderer, -12.07, 44.28, -19.12, 21.45,
                                        -31.46, -1.42);
            rr_renderer_bezier_curve_to(renderer, -43.79, -24.29, -59.23,
                                        -43.16, -71.19, -49.99);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -71.02, -45.90);
            rr_renderer_bezier_curve_to(renderer, -72.01, -47.74, -71.74,
                                        -49.74, -70.40, -50.37);
            rr_renderer_bezier_curve_to(renderer, -69.77, -50.67, -68.96,
                                        -50.61, -68.17, -50.20);
            rr_renderer_bezier_curve_to(renderer, -67.38, -49.79, -66.67,
                                        -49.06, -66.20, -48.18);
            rr_renderer_bezier_curve_to(renderer, -65.20, -46.33, -65.48,
                                        -44.33, -66.81, -43.70);
            rr_renderer_bezier_curve_to(renderer, -68.14, -43.07, -70.03,
                                        -44.05, -71.02, -45.90);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -16.73, 54.78);
            rr_renderer_bezier_curve_to(renderer, -17.72, 52.94, -17.45, 50.94,
                                        -16.12, 50.31);
            rr_renderer_bezier_curve_to(renderer, -15.48, 50.00, -14.67, 50.06,
                                        -13.89, 50.48);
            rr_renderer_bezier_curve_to(renderer, -13.10, 50.89, -12.38, 51.61,
                                        -11.91, 52.50);
            rr_renderer_bezier_curve_to(renderer, -10.92, 54.34, -11.19, 56.34,
                                        -12.52, 56.97);
            rr_renderer_bezier_curve_to(renderer, -13.85, 57.60, -15.74, 56.62,
                                        -16.73, 54.78);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -70.77, -45.61);
            rr_renderer_bezier_curve_to(renderer, -60.36, -50.52, -39.89,
                                        -32.21, -25.06, -4.71);
            rr_renderer_bezier_curve_to(renderer, -10.24, 22.79, -6.66, 49.07,
                                        -17.07, 53.98);
            rr_renderer_bezier_curve_to(renderer, -31.55, 19.82, -50.01, -14.41,
                                        -70.77, -45.61);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -69.85, -47.54);
            rr_renderer_bezier_curve_to(renderer, -60.46, -51.98, -40.55,
                                        -32.78, -25.39, -4.65);
            rr_renderer_bezier_curve_to(renderer, -10.22, 23.47, -5.54, 49.87,
                                        -14.93, 54.31);
            rr_renderer_bezier_curve_to(renderer, -16.66, 39.34, -23.80, 19.30,
                                        -34.42, -0.38);
            rr_renderer_bezier_curve_to(renderer, -45.03, -20.06, -58.01,
                                        -37.34, -69.85, -47.54);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 71.19, -49.99);
            rr_renderer_bezier_curve_to(renderer, 61.70, -54.48, 41.04, -34.08,
                                        25.06, -4.44);
            rr_renderer_bezier_curve_to(renderer, 9.07, 25.20, 3.81, 52.86,
                                        13.30, 57.34);
            rr_renderer_bezier_curve_to(renderer, 12.07, 44.28, 19.13, 21.45,
                                        31.46, -1.42);
            rr_renderer_bezier_curve_to(renderer, 43.79, -24.29, 59.23, -43.16,
                                        71.19, -49.99);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 71.02, -45.90);
            rr_renderer_bezier_curve_to(renderer, 72.01, -47.74, 71.74, -49.74,
                                        70.41, -50.37);
            rr_renderer_bezier_curve_to(renderer, 69.77, -50.67, 68.97, -50.61,
                                        68.18, -50.20);
            rr_renderer_bezier_curve_to(renderer, 67.39, -49.79, 66.68, -49.06,
                                        66.20, -48.18);
            rr_renderer_bezier_curve_to(renderer, 65.21, -46.34, 65.48, -44.33,
                                        66.81, -43.70);
            rr_renderer_bezier_curve_to(renderer, 68.14, -43.07, 70.03, -44.05,
                                        71.02, -45.90);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 16.73, 54.78);
            rr_renderer_bezier_curve_to(renderer, 17.72, 52.94, 17.45, 50.94,
                                        16.12, 50.31);
            rr_renderer_bezier_curve_to(renderer, 15.48, 50.00, 14.68, 50.06,
                                        13.89, 50.48);
            rr_renderer_bezier_curve_to(renderer, 13.10, 50.89, 12.39, 51.61,
                                        11.91, 52.50);
            rr_renderer_bezier_curve_to(renderer, 10.92, 54.34, 11.19, 56.34,
                                        12.52, 56.97);
            rr_renderer_bezier_curve_to(renderer, 13.86, 57.60, 15.74, 56.62,
                                        16.73, 54.78);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 70.77, -45.61);
            rr_renderer_bezier_curve_to(renderer, 60.36, -50.52, 39.89, -32.21,
                                        25.07, -4.71);
            rr_renderer_bezier_curve_to(renderer, 10.24, 22.79, 6.66, 49.07,
                                        17.08, 53.98);
            rr_renderer_bezier_curve_to(renderer, 31.55, 19.82, 50.01, -14.41,
                                        70.77, -45.61);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff46555e);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 69.85, -47.54);
            rr_renderer_bezier_curve_to(renderer, 60.46, -51.98, 40.56, -32.78,
                                        25.39, -4.65);
            rr_renderer_bezier_curve_to(renderer, 10.22, 23.47, 5.54, 49.87,
                                        14.93, 54.31);
            rr_renderer_bezier_curve_to(renderer, 16.66, 39.34, 23.80, 19.30,
                                        34.42, -0.38);
            rr_renderer_bezier_curve_to(renderer, 45.03, -20.07, 58.02, -37.35,
                                        69.85, -47.54);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_droplet: // kYinYang
            {
                float r = 10.0f; // Default radius
                rr_renderer_set_line_width(renderer, 3);
                rr_renderer_set_fill(renderer, 0xffffffff);
                rr_renderer_set_stroke(renderer, 0xffcfcfcf);
                rr_renderer_begin_path(renderer);
                rr_renderer_partial_arc(renderer, 0, 0, r, M_PI / 2, 3 * M_PI / 2, 0);
                rr_renderer_partial_arc(renderer, 0, -r / 2, r / 2, -M_PI / 2, M_PI / 2, 0);
                rr_renderer_partial_arc(renderer, 0, r / 2, r / 2, -M_PI / 2, M_PI / 2, 1);
                rr_renderer_fill(renderer);
                rr_renderer_stroke(renderer);
                rr_renderer_set_fill(renderer, 0xff333333);
                rr_renderer_set_stroke(renderer, 0xff292929);
                rr_renderer_begin_path(renderer);
                rr_renderer_partial_arc(renderer, 0, 0, r, -M_PI / 2, M_PI / 2, 0);
                rr_renderer_partial_arc(renderer, 0, r / 2, r / 2, M_PI / 2, 3 * M_PI / 2, 0);
                rr_renderer_partial_arc(renderer, 0, -r / 2, r / 2, M_PI / 2, 3 * M_PI / 2, 1);
                rr_renderer_fill(renderer);
                rr_renderer_stroke(renderer);
                rr_renderer_set_stroke(renderer, 0xffcfcfcf);
                rr_renderer_begin_path(renderer);
                rr_renderer_partial_arc(renderer, 0, 0, r, M_PI, 3 * M_PI / 2, 0);
                rr_renderer_partial_arc(renderer, 0, -r / 2, r / 2, -M_PI / 2, M_PI / 2, 0);
                rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_beak:
            rr_renderer_scale(renderer, 0.1);
            rr_renderer_set_fill(renderer, 0xff6e6054);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -63.37, -16.61);
            rr_renderer_bezier_curve_to(renderer, -53.28, -50.39, -43.09,
                                        -80.60, -33.69, -102.36);
            rr_renderer_bezier_curve_to(renderer, -24.29, -124.13, -14.99,
                                        -141.61, -6.98, -147.21);
            rr_renderer_bezier_curve_to(renderer, 1.04, -152.80, 6.58, -148.00,
                                        14.39, -135.93);
            rr_renderer_bezier_curve_to(renderer, 22.21, -123.86, 31.81, -95.76,
                                        39.92, -74.79);
            rr_renderer_bezier_curve_to(renderer, 48.03, -53.81, 53.97, -35.11,
                                        63.07, -10.08);
            rr_renderer_bezier_curve_to(renderer, 72.17, 14.95, 90.77, 54.72,
                                        94.53, 75.40);
            rr_renderer_bezier_curve_to(renderer, 98.29, 96.08, 94.63, 105.97,
                                        85.63, 113.98);
            rr_renderer_bezier_curve_to(renderer, 76.63, 122.00, 53.77, 122.79,
                                        40.51, 123.48);
            rr_renderer_bezier_curve_to(renderer, 27.25, 124.17, 14.89, 117.50,
                                        6.08, 118.14);
            rr_renderer_bezier_curve_to(renderer, -2.72, 118.78, -3.81, 122.96,
                                        -12.32, 127.32);
            rr_renderer_bezier_curve_to(renderer, -20.83, 131.67, -32.50,
                                        141.14, -44.97, 144.26);
            rr_renderer_bezier_curve_to(renderer, -57.43, 147.38, -78.90,
                                        153.36, -87.12, 146.04);
            rr_renderer_bezier_curve_to(renderer, -95.33, 138.72, -98.20,
                                        127.44, -94.24, 100.33);
            rr_renderer_bezier_curve_to(renderer, -90.28, 73.22, -73.46, 17.17,
                                        -63.37, -16.61);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff7d6e62);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 77.90, 96.01);
            rr_renderer_bezier_curve_to(renderer, 85.82, 79.37, 52.18, 17.16,
                                        40.01, -19.07);
            rr_renderer_bezier_curve_to(renderer, 27.83, -55.30, 11.81, -108.44,
                                        4.83, -121.36);
            rr_renderer_bezier_curve_to(renderer, -2.16, -134.29, 0.15, -130.33,
                                        -1.91, -96.63);
            rr_renderer_bezier_curve_to(renderer, -3.97, -62.94, -20.83, 48.69,
                                        -7.53, 80.80);
            rr_renderer_bezier_curve_to(renderer, 5.77, 112.91, 69.98, 112.66,
                                        77.90, 96.01);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff938479);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -76.43, 127.36);
            rr_renderer_bezier_curve_to(renderer, -84.26, 111.31, -52.73, 22.38,
                                        -40.95, -19.07);
            rr_renderer_bezier_curve_to(renderer, -29.17, -60.53, -13.02,
                                        -108.17, -5.77, -121.36);
            rr_renderer_bezier_curve_to(renderer, 1.48, -134.55, 0.55, -131.31,
                                        2.52, -98.21);
            rr_renderer_bezier_curve_to(renderer, 4.50, -65.11, 19.24, 39.65,
                                        6.08, 77.24);
            rr_renderer_bezier_curve_to(renderer, -7.07, 114.84, -68.59, 143.42,
                                        -76.43, 127.36);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_lightning:
            rr_renderer_scale(renderer, 0.18f);
            rr_renderer_set_fill(renderer, 0xffcbdce8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -71.99, 13.19);
            rr_renderer_line_to(renderer, -41.80, 3.64);
            rr_renderer_line_to(renderer, -43.33, 20.46);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffcbdce8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 71.61, -17.58);
            rr_renderer_line_to(renderer, 55.94, -16.82);
            rr_renderer_line_to(renderer, 62.06, -1.91);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffcbdce8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -6.39, 66.20);
            rr_renderer_line_to(renderer, 16.48, 53.14);
            rr_renderer_line_to(renderer, 39.07, 47.38);
            rr_renderer_line_to(renderer, 58.99, 24.95);
            rr_renderer_line_to(renderer, 2.16, 40.33);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffcbdce8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 46.56, -66.51);
            rr_renderer_line_to(renderer, 24.39, -49.69);
            rr_renderer_line_to(renderer, 39.68, -37.08);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffcbdce8);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 5.66, -44.72);
            rr_renderer_line_to(renderer, 11.39, -56.57);
            rr_renderer_line_to(renderer, -34.47, -62.30);
            rr_renderer_line_to(renderer, -57.41, -42.81);
            rr_renderer_line_to(renderer, -51.67, -2.29);
            rr_renderer_line_to(renderer, -34.15, -30.00);
            rr_renderer_line_to(renderer, -18.80, -44.72);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff95a4ad);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -54.03, -16.24);
            rr_renderer_line_to(renderer, -20.39, -49.11);
            rr_renderer_line_to(renderer, 23.18, -59.05);
            rr_renderer_line_to(renderer, 66.76, -32.67);
            rr_renderer_line_to(renderer, 50.70, 19.69);
            rr_renderer_line_to(renderer, -4.34, 60.59);
            rr_renderer_line_to(renderer, -49.82, 51.80);
            rr_renderer_line_to(renderer, -50.21, 16.63);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffabbbc5);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -36.83, -12.03);
            rr_renderer_line_to(renderer, -11.73, -35.29);
            rr_renderer_line_to(renderer, 19.36, -42.38);
            rr_renderer_line_to(renderer, 45.35, -25.79);
            rr_renderer_line_to(renderer, 35.80, 11.67);
            rr_renderer_line_to(renderer, -8.54, 43.39);
            rr_renderer_line_to(renderer, -33.39, 39.19);
            rr_renderer_line_to(renderer, -33.01, 13.96);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffe5f5ff);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -14.98, 25.80);
            rr_renderer_line_to(renderer, -6.57, -12.43);
            rr_renderer_line_to(renderer, 28.59, -34.21);
            rr_renderer_line_to(renderer, 46.56, -66.32);
            rr_renderer_line_to(renderer, 45.41, -22.37);
            rr_renderer_line_to(renderer, 7.19, 0.19);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffe5f5ff);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 41.97, 22.17);
            rr_renderer_line_to(renderer, 11.78, 20.64);
            rr_renderer_line_to(renderer, -12.37, 45.30);
            rr_renderer_line_to(renderer, -35.30, 54.86);
            rr_renderer_line_to(renderer, -20.01, 60.21);
            rr_renderer_line_to(renderer, -6.19, 66.51);
            rr_renderer_line_to(renderer, 20.19, 33.64);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffe5f5ff);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -53.97, -15.67);
            rr_renderer_line_to(renderer, -36.00, -15.29);
            rr_renderer_line_to(renderer, -25.68, 5.35);
            rr_renderer_line_to(renderer, -21.10, -22.55);
            rr_renderer_line_to(renderer, -57.41, -43.19);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffe5f5ff);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 72.00, -17.58);
            rr_renderer_line_to(renderer, 52.59, -3.14);
            rr_renderer_line_to(renderer, 30.92, 5.26);
            rr_renderer_line_to(renderer, 54.14, 9.37);
            rr_renderer_line_to(renderer, 65.99, -0.57);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffe5f5ff);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -71.61, 13.19);
            rr_renderer_line_to(renderer, -49.89, 30.01);
            rr_renderer_line_to(renderer, -23.83, 18.28);
            rr_renderer_line_to(renderer, -48.16, 16.98);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_third_eye:
            rr_renderer_scale(renderer, 0.6f);
            rr_renderer_set_fill(renderer, 0xff222222);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 1, 15);
            rr_renderer_quadratic_curve_to(renderer, 16, 0, 1, -15);
            rr_renderer_quadratic_curve_to(renderer, 0, -16, -1, -15);
            rr_renderer_quadratic_curve_to(renderer, -16, 0, -1, 15);
            rr_renderer_quadratic_curve_to(renderer, 0, 16, 1, 15);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffffffff);
            rr_renderer_begin_path(renderer);
            rr_renderer_arc(renderer, 0, 0, 7);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_mandible:
            rr_renderer_scale(renderer, 0.09);
            rr_renderer_set_fill(renderer, 0xff171612);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 90.08, 51.32);
            rr_renderer_bezier_curve_to(renderer, 89.87, 37.93, 86.16, 23.54,
                                        70.49, 4.15);
            rr_renderer_bezier_curve_to(renderer, 54.82, -15.23, 21.03, -50.36,
                                        -3.94, -65.00);
            rr_renderer_bezier_curve_to(renderer, -28.90, -79.63, -68.44,
                                        -91.72, -79.28, -83.66);
            rr_renderer_bezier_curve_to(renderer, -90.13, -75.60, -71.61,
                                        -37.86, -69.01, -16.64);
            rr_renderer_bezier_curve_to(renderer, -66.40, 4.57, -73.20, 31.43,
                                        -63.67, 43.64);
            rr_renderer_bezier_curve_to(renderer, -54.14, 55.85, -27.35, 48.78,
                                        -11.85, 56.62);
            rr_renderer_bezier_curve_to(renderer, 3.64, 64.47, 15.37, 86.06,
                                        29.30, 90.71);
            rr_renderer_bezier_curve_to(renderer, 43.23, 95.36, 61.60, 91.07,
                                        71.73, 84.50);
            rr_renderer_bezier_curve_to(renderer, 81.86, 77.94, 90.29, 64.71,
                                        90.08, 51.32);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff2b2820);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 67.91, 34.98);
            rr_renderer_bezier_curve_to(renderer, 64.36, 25.07, 55.54, 17.40,
                                        42.78, 4.24);
            rr_renderer_bezier_curve_to(renderer, 30.03, -8.92, 8.50, -32.65,
                                        -8.65, -43.98);
            rr_renderer_bezier_curve_to(renderer, -25.79, -55.30, -52.98,
                                        -68.08, -60.09, -63.71);
            rr_renderer_bezier_curve_to(renderer, -67.21, -59.34, -53.11,
                                        -33.24, -51.34, -17.75);
            rr_renderer_bezier_curve_to(renderer, -49.56, -2.25, -57.25, 20.24,
                                        -49.45, 29.27);
            rr_renderer_bezier_curve_to(renderer, -41.65, 38.30, -18.23, 29.59,
                                        -4.53, 36.45);
            rr_renderer_bezier_curve_to(renderer, 9.16, 43.31, 21.30, 65.88,
                                        32.73, 70.42);
            rr_renderer_bezier_curve_to(renderer, 44.17, 74.96, 58.21, 69.61,
                                        64.07, 63.70);
            rr_renderer_bezier_curve_to(renderer, 69.94, 57.79, 71.46, 44.89,
                                        67.91, 34.98);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_wax:
            rr_renderer_scale(renderer, 0.16);
            rr_renderer_set_fill(renderer, 0xffe1d5c3);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -44.77, 0.00);
            rr_renderer_bezier_curve_to(renderer, -44.77, -24.72, -24.73,
                                        -44.77, -0.00, -44.77);
            rr_renderer_bezier_curve_to(renderer, 11.87, -44.77, 23.26, -40.05,
                                        31.65, -31.66);
            rr_renderer_bezier_curve_to(renderer, 40.05, -23.26, 44.77, -11.87,
                                        44.77, 0.00);
            rr_renderer_bezier_curve_to(renderer, 44.77, 24.72, 24.72, 44.77,
                                        -0.00, 44.77);
            rr_renderer_bezier_curve_to(renderer, -24.73, 44.77, -44.77, 24.72,
                                        -44.77, 0.00);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -26.72, -59.91);
            rr_renderer_line_to(renderer, 26.73, -59.91);
            rr_renderer_line_to(renderer, 26.73, -32.29);
            rr_renderer_line_to(renderer, -26.72, -32.29);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 26.72, 59.92);
            rr_renderer_line_to(renderer, -26.72, 59.92);
            rr_renderer_line_to(renderer, -26.72, 32.35);
            rr_renderer_line_to(renderer, 26.72, 32.35);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 19.82, -57.88);
            rr_renderer_bezier_curve_to(renderer, 26.43, -61.69, 34.88, -59.43,
                                        38.69, -52.82);
            rr_renderer_bezier_curve_to(renderer, 40.52, -49.65, 41.02, -45.88,
                                        40.07, -42.34);
            rr_renderer_bezier_curve_to(renderer, 39.12, -38.81, 36.80, -35.79,
                                        33.63, -33.96);
            rr_renderer_bezier_curve_to(renderer, 27.03, -30.15, 18.58, -32.41,
                                        14.77, -39.02);
            rr_renderer_bezier_curve_to(renderer, 10.95, -45.62, 13.22, -54.07,
                                        19.82, -57.88);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 38.53, -53.09);
            rr_renderer_line_to(renderer, 65.25, -6.80);
            rr_renderer_line_to(renderer, 41.28, 7.03);
            rr_renderer_line_to(renderer, 14.56, -39.26);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -19.83, 57.89);
            rr_renderer_bezier_curve_to(renderer, -26.43, 61.70, -34.88, 59.43,
                                        -38.69, 52.83);
            rr_renderer_bezier_curve_to(renderer, -40.52, 49.66, -41.02, 45.89,
                                        -40.07, 42.35);
            rr_renderer_bezier_curve_to(renderer, -39.12, 38.81, -36.81, 35.80,
                                        -33.63, 33.96);
            rr_renderer_bezier_curve_to(renderer, -27.03, 30.15, -18.58, 32.42,
                                        -14.77, 39.02);
            rr_renderer_bezier_curve_to(renderer, -10.96, 45.63, -13.22, 54.07,
                                        -19.83, 57.89);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -46.23, 12.14);
            rr_renderer_bezier_curve_to(renderer, -52.84, 15.95, -61.28, 13.69,
                                        -65.10, 7.08);
            rr_renderer_bezier_curve_to(renderer, -66.93, 3.91, -67.42, 0.14,
                                        -66.48, -3.40);
            rr_renderer_bezier_curve_to(renderer, -65.53, -6.94, -63.21, -9.95,
                                        -60.04, -11.78);
            rr_renderer_bezier_curve_to(renderer, -53.43, -15.60, -44.99,
                                        -13.33, -41.18, -6.73);
            rr_renderer_bezier_curve_to(renderer, -37.36, -0.12, -39.63, 8.32,
                                        -46.23, 12.14);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -38.54, 53.10);
            rr_renderer_line_to(renderer, -65.25, 6.81);
            rr_renderer_line_to(renderer, -41.35, -6.99);
            rr_renderer_line_to(renderer, -14.63, 39.30);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -33.63, -33.97);
            rr_renderer_bezier_curve_to(renderer, -40.24, -37.78, -42.50,
                                        -46.22, -38.69, -52.83);
            rr_renderer_bezier_curve_to(renderer, -36.86, -56.00, -33.84,
                                        -58.32, -30.30, -59.27);
            rr_renderer_bezier_curve_to(renderer, -26.77, -60.21, -23.00,
                                        -59.72, -19.83, -57.89);
            rr_renderer_bezier_curve_to(renderer, -13.22, -54.07, -10.96,
                                        -45.63, -14.77, -39.02);
            rr_renderer_bezier_curve_to(renderer, -18.58, -32.42, -27.03,
                                        -30.15, -33.63, -33.97);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -65.25, -6.81);
            rr_renderer_line_to(renderer, -38.53, -53.10);
            rr_renderer_line_to(renderer, -14.65, -39.32);
            rr_renderer_line_to(renderer, -41.37, 6.97);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 60.04, -11.79);
            rr_renderer_bezier_curve_to(renderer, 66.64, -7.98, 68.91, 0.47,
                                        65.10, 7.07);
            rr_renderer_bezier_curve_to(renderer, 63.27, 10.25, 60.25, 12.56,
                                        56.71, 13.51);
            rr_renderer_bezier_curve_to(renderer, 53.17, 14.46, 49.40, 13.96,
                                        46.23, 12.13);
            rr_renderer_bezier_curve_to(renderer, 39.63, 8.32, 37.36, -0.13,
                                        41.17, -6.73);
            rr_renderer_bezier_curve_to(renderer, 44.99, -13.34, 53.43, -15.60,
                                        60.04, -11.79);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 33.63, 33.96);
            rr_renderer_bezier_curve_to(renderer, 40.24, 37.77, 42.50, 46.22,
                                        38.69, 52.82);
            rr_renderer_bezier_curve_to(renderer, 36.86, 55.99, 33.84, 58.31,
                                        30.31, 59.26);
            rr_renderer_bezier_curve_to(renderer, 26.77, 60.21, 23.00, 59.71,
                                        19.83, 57.88);
            rr_renderer_bezier_curve_to(renderer, 13.22, 54.07, 10.96, 45.62,
                                        14.77, 39.02);
            rr_renderer_bezier_curve_to(renderer, 18.58, 32.41, 27.03, 30.15,
                                        33.63, 33.96);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 65.25, 6.80);
            rr_renderer_line_to(renderer, 38.53, 53.09);
            rr_renderer_line_to(renderer, 14.76, 39.37);
            rr_renderer_line_to(renderer, 41.48, -6.92);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -30.85, 11.25);
            rr_renderer_bezier_curve_to(renderer, -37.06, 11.25, -42.09, 6.21,
                                        -42.09, -0.00);
            rr_renderer_bezier_curve_to(renderer, -42.09, -6.21, -37.06, -11.25,
                                        -30.85, -11.25);
            rr_renderer_bezier_curve_to(renderer, -35.73, -4.55, -35.73, 4.55,
                                        -30.85, 11.25);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 30.85, -11.25);
            rr_renderer_bezier_curve_to(renderer, 37.06, -11.25, 42.09, -6.21,
                                        42.09, -0.00);
            rr_renderer_bezier_curve_to(renderer, 42.09, 6.21, 37.06, 11.25,
                                        30.85, 11.25);
            rr_renderer_bezier_curve_to(renderer, 35.73, 4.55, 35.73, -4.55,
                                        30.85, -11.25);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -5.67, 32.34);
            rr_renderer_bezier_curve_to(renderer, -8.78, 37.72, -15.66, 39.56,
                                        -21.04, 36.45);
            rr_renderer_bezier_curve_to(renderer, -26.42, 33.35, -28.27, 26.47,
                                        -25.17, 21.09);
            rr_renderer_bezier_curve_to(renderer, -21.80, 28.67, -13.92, 33.21,
                                        -5.67, 32.34);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 5.67, -32.34);
            rr_renderer_bezier_curve_to(renderer, 8.77, -37.72, 15.65, -39.56,
                                        21.04, -36.45);
            rr_renderer_bezier_curve_to(renderer, 26.42, -33.35, 28.27, -26.47,
                                        25.17, -21.09);
            rr_renderer_bezier_curve_to(renderer, 21.80, -28.67, 13.92, -33.21,
                                        5.67, -32.34);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -25.15, -21.10);
            rr_renderer_bezier_curve_to(renderer, -28.26, -26.49, -26.41,
                                        -33.37, -21.02, -36.47);
            rr_renderer_bezier_curve_to(renderer, -15.64, -39.57, -8.76, -37.72,
                                        -5.66, -32.34);
            rr_renderer_bezier_curve_to(renderer, -13.90, -33.22, -21.78,
                                        -28.68, -25.15, -21.10);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xffc9bdac);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 25.15, 21.10);
            rr_renderer_bezier_curve_to(renderer, 28.25, 26.49, 26.40, 33.37,
                                        21.02, 36.47);
            rr_renderer_bezier_curve_to(renderer, 15.64, 39.57, 8.76, 37.72,
                                        5.65, 32.34);
            rr_renderer_bezier_curve_to(renderer, 13.90, 33.22, 21.78, 28.68,
                                        25.15, 21.10);
            rr_renderer_fill(renderer);
            break;
        case rr_petal_id_sand: // kSand
            {
            rr_renderer_set_fill(renderer, 0xffe0c85c);
            rr_renderer_set_stroke(renderer, 0xffb5a24b);
            rr_renderer_set_line_width(renderer, 3);
                rr_renderer_set_line_cap(renderer, 1);
                rr_renderer_set_line_join(renderer, 1);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 7, 0);
                rr_renderer_line_to(renderer, 3.5f, 6.062f);
                rr_renderer_line_to(renderer, -3.5f, 6.062f);
                rr_renderer_line_to(renderer, -7, 0);
                rr_renderer_line_to(renderer, -3.5f, -6.062f);
                rr_renderer_line_to(renderer, 3.5f, -6.062f);
                // Close path
            rr_renderer_line_to(renderer, 7, 0);
            rr_renderer_fill(renderer);
            rr_renderer_stroke(renderer);
            }
            break;
        case rr_petal_id_mint:
            rr_renderer_scale(renderer, 0.2f);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -54.13, 0.77);
            rr_renderer_bezier_curve_to(renderer, -64.26, -16.45, -58.52,
                                        -38.62, -41.30, -48.75);
            rr_renderer_bezier_curve_to(renderer, -33.04, -53.61, -23.17,
                                        -54.99, -13.89, -52.59);
            rr_renderer_bezier_curve_to(renderer, -4.60, -50.18, 3.35, -44.18,
                                        8.21, -35.91);
            rr_renderer_bezier_curve_to(renderer, 18.34, -18.70, 12.60, 3.47,
                                        -4.62, 13.60);
            rr_renderer_bezier_curve_to(renderer, -21.84, 23.73, -44.00, 17.98,
                                        -54.13, 0.77);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -53.89, -44.95);
            rr_renderer_bezier_curve_to(renderer, -57.47, -51.03, -55.44,
                                        -58.86, -49.36, -62.44);
            rr_renderer_bezier_curve_to(renderer, -46.44, -64.16, -42.96,
                                        -64.65, -39.68, -63.80);
            rr_renderer_bezier_curve_to(renderer, -36.40, -62.95, -33.59,
                                        -60.83, -31.87, -57.91);
            rr_renderer_bezier_curve_to(renderer, -28.30, -51.83, -30.32,
                                        -44.00, -36.41, -40.42);
            rr_renderer_bezier_curve_to(renderer, -42.49, -36.84, -50.32,
                                        -38.87, -53.89, -44.95);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -59.61, -33.85);
            rr_renderer_bezier_curve_to(renderer, -58.42, -48.45, -54.66,
                                        -60.06, -51.21, -59.78);
            rr_renderer_bezier_curve_to(renderer, -49.55, -59.64, -48.19,
                                        -56.73, -47.42, -51.67);
            rr_renderer_bezier_curve_to(renderer, -46.65, -46.62, -46.54,
                                        -39.84, -47.11, -32.83);
            rr_renderer_bezier_curve_to(renderer, -48.30, -18.23, -52.07, -6.62,
                                        -55.52, -6.90);
            rr_renderer_bezier_curve_to(renderer, -57.17, -7.04, -58.54, -9.95,
                                        -59.30, -15.01);
            rr_renderer_bezier_curve_to(renderer, -60.07, -20.06, -60.18,
                                        -26.84, -59.61, -33.85);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -19.60, -57.40);
            rr_renderer_bezier_curve_to(renderer, -32.94, -63.45, -44.91,
                                        -65.80, -46.34, -62.64);
            rr_renderer_bezier_curve_to(renderer, -47.03, -61.13, -45.14,
                                        -58.52, -41.10, -55.39);
            rr_renderer_bezier_curve_to(renderer, -37.05, -52.27, -31.18,
                                        -48.88, -24.77, -45.97);
            rr_renderer_bezier_curve_to(renderer, -11.43, -39.92, 0.55, -37.57,
                                        1.98, -40.73);
            rr_renderer_bezier_curve_to(renderer, 2.66, -42.24, 0.78, -44.85,
                                        -3.27, -47.98);
            rr_renderer_bezier_curve_to(renderer, -7.32, -51.10, -13.19, -54.49,
                                        -19.60, -57.40);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff56d02a);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -43.18, -5.68);
            rr_renderer_bezier_curve_to(renderer, -49.72, -16.79, -46.01,
                                        -31.10, -34.90, -37.64);
            rr_renderer_bezier_curve_to(renderer, -29.56, -40.78, -23.19,
                                        -41.67, -17.20, -40.12);
            rr_renderer_bezier_curve_to(renderer, -11.20, -38.57, -6.07, -34.70,
                                        -2.93, -29.36);
            rr_renderer_bezier_curve_to(renderer, 3.61, -18.24, -0.10, -3.93,
                                        -11.22, 2.61);
            rr_renderer_bezier_curve_to(renderer, -22.33, 9.15, -36.64, 5.44,
                                        -43.18, -5.68);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff56d02a);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -43.80, -6.99);
            rr_renderer_line_to(renderer, -44.42, -8.37);
            rr_renderer_line_to(renderer, -45.31, -10.28);
            rr_renderer_line_to(renderer, -46.14, -12.34);
            rr_renderer_line_to(renderer, -46.78, -14.27);
            rr_renderer_line_to(renderer, -47.41, -16.39);
            rr_renderer_line_to(renderer, -47.89, -18.47);
            rr_renderer_line_to(renderer, -48.28, -20.73);
            rr_renderer_line_to(renderer, -48.53, -22.95);
            rr_renderer_line_to(renderer, -48.65, -25.60);
            rr_renderer_line_to(renderer, -48.51, -28.54);
            rr_renderer_line_to(renderer, -48.26, -31.95);
            rr_renderer_line_to(renderer, -47.83, -35.54);
            rr_renderer_line_to(renderer, -47.30, -38.68);
            rr_renderer_line_to(renderer, -46.50, -42.06);
            rr_renderer_line_to(renderer, -45.15, -46.86);
            rr_renderer_line_to(renderer, -43.31, -51.68);
            rr_renderer_line_to(renderer, -20.50, -20.63);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff56d02a);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -3.98, -30.42);
            rr_renderer_line_to(renderer, -4.88, -31.63);
            rr_renderer_line_to(renderer, -6.12, -33.33);
            rr_renderer_line_to(renderer, -7.52, -35.06);
            rr_renderer_line_to(renderer, -8.90, -36.55);
            rr_renderer_line_to(renderer, -10.45, -38.14);
            rr_renderer_line_to(renderer, -12.04, -39.57);
            rr_renderer_line_to(renderer, -13.82, -41.01);
            rr_renderer_line_to(renderer, -15.64, -42.31);
            rr_renderer_line_to(renderer, -17.89, -43.69);
            rr_renderer_line_to(renderer, -20.53, -45.00);
            rr_renderer_line_to(renderer, -23.63, -46.44);
            rr_renderer_line_to(renderer, -26.98, -47.81);
            rr_renderer_line_to(renderer, -29.98, -48.87);
            rr_renderer_line_to(renderer, -33.33, -49.81);
            rr_renderer_line_to(renderer, -38.18, -50.96);
            rr_renderer_line_to(renderer, -43.28, -51.70);
            rr_renderer_line_to(renderer, -27.22, -16.67);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, -32.92, -21.67);
            rr_renderer_bezier_curve_to(renderer, -34.71, -24.72, -33.69,
                                        -28.63, -30.65, -30.42);
            rr_renderer_bezier_curve_to(renderer, -29.19, -31.28, -27.45,
                                        -31.52, -25.81, -31.10);
            rr_renderer_bezier_curve_to(renderer, -24.17, -30.67, -22.77,
                                        -29.61, -21.91, -28.15);
            rr_renderer_line_to(renderer, -4.72, 1.06);
            rr_renderer_bezier_curve_to(renderer, -2.93, 4.10, -3.95, 8.01,
                                        -6.99, 9.80);
            rr_renderer_bezier_curve_to(renderer, -10.03, 11.59, -13.94, 10.58,
                                        -15.73, 7.54);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 37.60, -4.61);
            rr_renderer_bezier_curve_to(renderer, 54.34, 6.29, 59.09, 28.69,
                                        48.19, 45.43);
            rr_renderer_bezier_curve_to(renderer, 42.96, 53.47, 34.75, 59.11,
                                        25.36, 61.09);
            rr_renderer_bezier_curve_to(renderer, 15.98, 63.08, 6.19, 61.25,
                                        -1.85, 56.02);
            rr_renderer_bezier_curve_to(renderer, -18.59, 45.13, -23.33, 22.72,
                                        -12.44, 5.98);
            rr_renderer_bezier_curve_to(renderer, -1.54, -10.76, 20.86, -15.50,
                                        37.60, -4.61);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 57.77, 36.43);
            rr_renderer_bezier_curve_to(renderer, 63.68, 40.27, 65.36, 48.19,
                                        61.51, 54.10);
            rr_renderer_bezier_curve_to(renderer, 59.66, 56.94, 56.76, 58.93,
                                        53.45, 59.63);
            rr_renderer_bezier_curve_to(renderer, 50.13, 60.33, 46.67, 59.69,
                                        43.84, 57.84);
            rr_renderer_bezier_curve_to(renderer, 37.92, 53.99, 36.25, 46.08,
                                        40.10, 40.17);
            rr_renderer_bezier_curve_to(renderer, 43.94, 34.25, 51.86, 32.58,
                                        57.77, 36.43);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 57.94, 23.94);
            rr_renderer_bezier_curve_to(renderer, 63.38, 37.54, 65.19, 49.61,
                                        61.97, 50.90);
            rr_renderer_bezier_curve_to(renderer, 60.43, 51.51, 57.91, 49.51,
                                        54.97, 45.33);
            rr_renderer_bezier_curve_to(renderer, 52.03, 41.15, 48.91, 35.13,
                                        46.29, 28.60);
            rr_renderer_bezier_curve_to(renderer, 40.85, 14.99, 39.04, 2.93,
                                        42.26, 1.64);
            rr_renderer_bezier_curve_to(renderer, 43.80, 1.02, 46.32, 3.03,
                                        49.26, 7.21);
            rr_renderer_bezier_curve_to(renderer, 52.20, 11.39, 55.32, 17.41,
                                        57.94, 23.94);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 32.62, 62.85);
            rr_renderer_bezier_curve_to(renderer, 47.26, 62.32, 59.02, 59.09,
                                        58.90, 55.63);
            rr_renderer_bezier_curve_to(renderer, 58.84, 53.96, 55.98, 52.47,
                                        50.97, 51.48);
            rr_renderer_bezier_curve_to(renderer, 45.95, 50.48, 39.19, 50.07,
                                        32.16, 50.32);
            rr_renderer_bezier_curve_to(renderer, 17.52, 50.85, 5.75, 54.09,
                                        5.88, 57.55);
            rr_renderer_bezier_curve_to(renderer, 5.94, 59.21, 8.79, 60.70,
                                        13.80, 61.70);
            rr_renderer_bezier_curve_to(renderer, 18.82, 62.69, 25.59, 63.11,
                                        32.62, 62.85);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff56d02a);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 30.67, 6.04);
            rr_renderer_bezier_curve_to(renderer, 41.48, 13.08, 44.54, 27.54,
                                        37.51, 38.35);
            rr_renderer_bezier_curve_to(renderer, 34.13, 43.54, 28.83, 47.18,
                                        22.77, 48.46);
            rr_renderer_bezier_curve_to(renderer, 16.71, 49.74, 10.39, 48.56,
                                        5.20, 45.19);
            rr_renderer_bezier_curve_to(renderer, -5.61, 38.15, -8.67, 23.69,
                                        -1.63, 12.88);
            rr_renderer_bezier_curve_to(renderer, 5.40, 2.07, 19.86, -0.99,
                                        30.67, 6.04);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff56d02a);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 31.81, 6.94);
            rr_renderer_line_to(renderer, 32.98, 7.90);
            rr_renderer_line_to(renderer, 34.62, 9.21);
            rr_renderer_line_to(renderer, 36.29, 10.69);
            rr_renderer_line_to(renderer, 37.72, 12.13);
            rr_renderer_line_to(renderer, 39.23, 13.75);
            rr_renderer_line_to(renderer, 40.59, 15.40);
            rr_renderer_line_to(renderer, 41.95, 17.25);
            rr_renderer_line_to(renderer, 43.16, 19.13);
            rr_renderer_line_to(renderer, 44.45, 21.44);
            rr_renderer_line_to(renderer, 45.64, 24.14);
            rr_renderer_line_to(renderer, 46.93, 27.29);
            rr_renderer_line_to(renderer, 48.15, 30.70);
            rr_renderer_line_to(renderer, 49.07, 33.75);
            rr_renderer_line_to(renderer, 49.86, 37.13);
            rr_renderer_line_to(renderer, 50.80, 42.03);
            rr_renderer_line_to(renderer, 51.30, 47.16);
            rr_renderer_line_to(renderer, 17.04, 29.54);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff56d02a);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 6.61, 45.67);
            rr_renderer_line_to(renderer, 7.96, 46.35);
            rr_renderer_line_to(renderer, 9.83, 47.32);
            rr_renderer_line_to(renderer, 11.85, 48.24);
            rr_renderer_line_to(renderer, 13.75, 48.97);
            rr_renderer_line_to(renderer, 15.84, 49.69);
            rr_renderer_line_to(renderer, 17.90, 50.27);
            rr_renderer_line_to(renderer, 20.14, 50.76);
            rr_renderer_line_to(renderer, 22.35, 51.11);
            rr_renderer_line_to(renderer, 24.98, 51.35);
            rr_renderer_line_to(renderer, 27.93, 51.34);
            rr_renderer_line_to(renderer, 31.34, 51.25);
            rr_renderer_line_to(renderer, 34.96, 50.98);
            rr_renderer_line_to(renderer, 38.11, 50.59);
            rr_renderer_line_to(renderer, 41.53, 49.94);
            rr_renderer_line_to(renderer, 46.38, 48.81);
            rr_renderer_line_to(renderer, 51.28, 47.19);
            rr_renderer_line_to(renderer, 21.29, 23.00);
            rr_renderer_fill(renderer);
            rr_renderer_set_fill(renderer, 0xff3eb114);
            rr_renderer_begin_path(renderer);
            rr_renderer_move_to(renderer, 28.62, 24.94);
            rr_renderer_bezier_curve_to(renderer, 31.58, 26.86, 32.41, 30.82,
                                        30.49, 33.78);
            rr_renderer_bezier_curve_to(renderer, 29.56, 35.20, 28.11, 36.19,
                                        26.46, 36.54);
            rr_renderer_bezier_curve_to(renderer, 24.80, 36.89, 23.07, 36.57,
                                        21.65, 35.65);
            rr_renderer_line_to(renderer, -6.76, 17.16);
            rr_renderer_bezier_curve_to(renderer, -9.71, 15.24, -10.55, 11.28,
                                        -8.63, 8.32);
            rr_renderer_bezier_curve_to(renderer, -6.70, 5.37, -2.75, 4.53,
                                        0.21, 6.45);
            rr_renderer_fill(renderer);
            break;
        default:
            break;
        }
    }
}

void rr_renderer_draw_static_petal(struct rr_renderer *renderer, uint8_t id,
                                   uint8_t rarity, uint8_t flags)
{
    struct rr_renderer_context_state state;
    rr_renderer_context_state_init(renderer, &state);
    uint32_t count = RR_PETAL_DATA[id].count[rarity];
    if (id == rr_petal_id_peas)
        rr_renderer_rotate(renderer, 1.0f - M_PI / 4.0f);
    if (count <= 1)
    {
        if (id == rr_petal_id_shell)
            rr_renderer_rotate(renderer, 1.0f);
        else if (id == rr_petal_id_leaf)
            rr_renderer_rotate(renderer, -1.0f);
        else if (id == rr_petal_id_magnet)
            rr_renderer_rotate(renderer, -1.0f);
        else if (id == rr_petal_id_bone)
            rr_renderer_rotate(renderer, -1.0f);
        else if (id == rr_petal_id_club)
            rr_renderer_rotate(renderer, -1.0f);
        else if (id == rr_petal_id_feather)
            rr_renderer_rotate(renderer, 0.5f);
        else if (id == rr_petal_id_beak)
            rr_renderer_rotate(renderer, 1.0f);
        else if (id == rr_petal_id_lightning)
            rr_renderer_rotate(renderer, 1.0f);
        else if (id == rr_petal_id_mandible)
            rr_renderer_rotate(renderer, -1.0f);
        else if (id == rr_petal_id_wax)
            rr_renderer_rotate(renderer, 0.3f);
        rr_renderer_draw_petal(renderer, id, flags);
    }
    else
    {
        float r = RR_PETAL_DATA[id].clump_radius == 0.0f
                      ? 10.0f
                      : RR_PETAL_DATA[id].clump_radius;
        for (uint32_t i = 0; i < count; ++i)
        {
            struct rr_renderer_context_state state;
            rr_renderer_context_state_init(renderer, &state);
            // Rotate to position around circle (matches C++ code order)
            rr_renderer_rotate(renderer, i * 2.0f * M_PI / count);
            // Translate outward
            rr_renderer_translate(renderer, r, 0.0f);
            // Rotate for icon angle (petal-specific orientation)
            if (id == rr_petal_id_shell)
                rr_renderer_rotate(renderer, 1.0f);
            else if (id == rr_petal_id_leaf)
                rr_renderer_rotate(renderer, -1.0f);
            else if (id == rr_petal_id_stinger && rarity >= rr_rarity_id_exotic)
                rr_renderer_rotate(renderer, M_PI);
            else if (id == rr_petal_id_wax)
                rr_renderer_rotate(renderer, 0.3f);
            rr_renderer_draw_petal(renderer, id, flags);
            rr_renderer_context_state_free(renderer, &state);
        }
    }
    rr_renderer_context_state_free(renderer, &state);
}

void rr_renderer_draw_petal_with_name(struct rr_renderer *renderer, uint8_t id,
                                      uint8_t rarity)
{
    rr_renderer_translate(renderer, 0, -5);
    rr_renderer_draw_static_petal(renderer, id, rarity, 1);
    rr_renderer_translate(renderer, 0, 25);
    rr_renderer_draw_petal_name(renderer, id, 12, 0, 0);
}

void rr_renderer_petal_cache_init()
{
    rr_renderer_init(&petal_cache);
    rr_renderer_set_dimensions(&petal_cache, IMAGE_SIZE * rr_petal_id_max,
                               IMAGE_SIZE);
    rr_renderer_translate(&petal_cache, IMAGE_SIZE / 2, IMAGE_SIZE / 2);
    struct rr_renderer_context_state state;
    for (uint32_t i = 0; i < rr_petal_id_max; ++i)
    {
        rr_renderer_context_state_init(&petal_cache, &state);
        rr_renderer_scale(&petal_cache, IMAGE_SIZE / 50);
        rr_renderer_draw_petal(&petal_cache, i, 0);
        rr_renderer_context_state_free(&petal_cache, &state);
        rr_renderer_translate(&petal_cache, IMAGE_SIZE, 0);
    }
}