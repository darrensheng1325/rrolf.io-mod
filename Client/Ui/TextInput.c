#include <Client/Ui/Ui.h>

#include <stdlib.h>
#include <string.h>

#include <Client/Game.h>
#include <Client/InputData.h>
#include <Client/Renderer/Renderer.h>

struct rr_ui_text_input_metadata
{
    char *text;
    uint8_t len;
    uint8_t max;
    uint8_t focused;
};

static void text_input_on_render(struct rr_ui_element *this, struct rr_game *game)
{
    struct rr_renderer_context_state state;
    struct rr_ui_text_input_metadata *data = this->data;
    struct rr_renderer *renderer = game->renderer;
    data->len = strlen(data->text);
    if (data->focused)
    {
        for (uint8_t i = 1; i < 255; i++)
            if (rr_bitset_get(game->input_data->keycodes_pressed_this_tick, i) && data->len < data->max)
                {
                    data->text[data->len++] = i;
                    data->text[data->len] = 0;
                }
        
        if (rr_bitset_get(game->input_data->keys_pressed_this_tick, 8) && data->len > 0)
            data->text[--data->len] = 0;
    }
    
    rr_renderer_scale(renderer, renderer->scale);
    if (game->input_data->mouse_buttons_up_this_tick & 1)
        data->focused = rr_ui_mouse_over(this, game);
    
    rr_renderer_set_fill(renderer, 0xffffffff);
    rr_renderer_set_stroke(renderer, 0xff222222);
    rr_renderer_set_line_width(renderer, this->height * 0.12);
    rr_renderer_begin_path(renderer);
    rr_renderer_fill_rect(renderer, -this->width / 2, -this->height / 2, this->width, this->height);
    rr_renderer_stroke_rect(renderer, -this->width / 2, -this->height / 2, this->width, this->height);
    rr_renderer_set_text_size(renderer, this->height * 0.8);
    rr_renderer_set_text_align(renderer, 0);
    rr_renderer_set_text_baseline(renderer, 1);
    rr_renderer_set_line_width(renderer, this->height * 0.8 * 0.12);
    rr_renderer_stroke_text(renderer, data->text, -this->width * 0.48, 0);
    rr_renderer_fill_text(renderer, data->text, -this->width * 0.48, 0);
}

struct rr_ui_element *rr_ui_text_input_init(float w, float h, char *text, uint8_t max_length)
{
    struct rr_ui_element *element = rr_ui_element_init();
    struct rr_ui_text_input_metadata *data = malloc(sizeof *data);
    memset(data, 0, sizeof *data);

    data->max = max_length;
    data->len = strlen(text);
    data->text = text;
    element->data = data;
    element->abs_width = element->width = w;
    element->abs_height = element->height = h;
    element->on_render = text_input_on_render;
    return element;
}