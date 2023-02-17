#ifndef TEXT_RENDERER_H
#define TEXT_RENDERER_H

#include "../include/stb_truetype.h"
#include "../include/glm/glm.hpp"
#include "pp_shaderer.h"

struct Font {
    const char* filename;
    unsigned int texture;
    stbtt_bakedchar* char_data;
    int first_char;
    int num_chars;
    int font_height;
    int bitmap_width;
    int bitmap_height;
};

struct TextRenderer {
    unsigned int vao;
    unsigned int vbo;
    unsigned int bytes_offset;
    unsigned int vertex_count;
    Font* fonts;
};

void text_render_init(TextRenderer *text_renderer);

void text_render_create_font_atlas(Font *font);

void text_render_add_queue(float x, float y, const char* text,
                           glm::vec3 c, Font* font);

void text_render_draw(Font* font, Shader s);

#endif
