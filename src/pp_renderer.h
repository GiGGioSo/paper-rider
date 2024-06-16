#ifndef PR_RENDERER_H
#define PR_RENDERER_H

#include "../include/glm/glm.hpp"
#include "../include/stb_truetype.h"
#include "pp_rect.h"
#include "pp_shaderer.h"

#include <iostream>

#define PR_TEX1_FRECCIA 0
#define PR_TEX1_PLANE 1
#define PR_LAST_TEX1 PR_TEX1_PLANE

struct TextureElement {
    char filename[256];
    int width;
    int height;
};

struct ArrayTexture {
    TextureElement *elements;
    int elements_len;
    unsigned int id;
};

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

struct Texture {
    unsigned int id;
    int width;
    int height;
    int nr_channels;
};
struct TexCoords {
    float tx;
    float ty;
    float tw;
    float th;
};

struct DataImage {
    uint8_t *data;
    int width;
    int height;
    int nr_channels;
    const char *path;
};

struct DataImages {
    DataImage *items;
    size_t count;
    size_t capacity;
};

struct Renderer {
    unsigned int uni_vao;
    unsigned int uni_vbo;
    unsigned int uni_bytes_offset;
    unsigned int uni_vertex_count;

    unsigned int tex_vao;
    unsigned int tex_vbo;
    unsigned int tex_bytes_offset;
    unsigned int tex_vertex_count;

    unsigned int array_tex_vao;
    unsigned int array_tex_vbo;
    unsigned int array_tex_bytes_offset;
    unsigned int array_tex_vertex_count;

    unsigned int text_vao;
    unsigned int text_vbo;
    unsigned int text_bytes_offset;
    unsigned int text_vertex_count;
};

TexCoords
texcoords_in_texture_space(size_t x, size_t y, size_t w, size_t h, Texture tex, bool inverse);

void
renderer_init(Renderer* renderer);

// NOTE: Unicolor rendering
void
renderer_add_queue_uni(float x, float y, float w, float h, float r, glm::vec4 c, bool triangle, bool centered);

inline void
renderer_add_queue_uni(Rect rec, glm::vec4 c, bool centered) {
    renderer_add_queue_uni(rec.pos.x, rec.pos.y,
                          rec.dim.x, rec.dim.y, rec.angle,
                          c, rec.triangle, centered);
}

void
renderer_draw_uni(Shader s);


// NOTE: Textured rendering
// This is intended to be used with a single texture containing everything
void
renderer_create_texture(Texture* t, const char* filename);

void
renderer_add_queue_tex(float x, float y, float w, float h, float r, bool centered, float tx, float ty, float tw, float th);

inline void
renderer_add_queue_tex(Rect rec, TexCoords t, bool centered) {
    renderer_add_queue_tex(rec.pos.x, rec.pos.y,
                           rec.dim.x, rec.dim.y,
                           rec.angle, centered,
                           t.tx, t.ty,
                           t.tw, t.th);
}

void
renderer_draw_tex(Shader s, Texture* t);

// NOTE: Texture rendering with array textures
void
renderer_add_queue_array_tex(ArrayTexture *at, int layer);
void
renderer_draw_array_tex(Shader s, ArrayTexture *at);

// NOTE: Text rendering
int
renderer_create_font_atlas(Font *font);

void
renderer_add_queue_text(float x, float y, const char* text, glm::vec4 c, Font* font, bool centered);

void
renderer_draw_text(Font* font, Shader s);

#endif // PR_RENDERER_H
