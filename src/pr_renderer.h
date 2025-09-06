#ifndef PR_RENDERER_H
#define PR_RENDERER_H

#include "../include/stb_truetype.h"
#include "pr_rect.h"
#include "pr_shaderer.h"
#include "pr_mathy.h"

#include <iostream>

#define PR_TEX1_FRECCIA 0
#define PR_LAST_TEX1 PR_TEX1_FRECCIA
#define PR_TEX2_PLANE 0
#define PR_LAST_TEX2 PR_TEX2_PLANE


#define PR_MAX_UNICOLOR_VERTICES (2000 * 6)

#define PR_MAX_TEXTURED_VERTICES (1000 * 6)

#define PR_MAX_TEXT_VERTICES (1000 * 6)

typedef struct PR_TexCoords {
    // Lower left corner is (0, 0)
    float tx;
    float ty;
    float tw;
    float th;
} PR_TexCoords;

typedef struct PR_TextureElement {
    char filename[256];
    int width;
    int height;
    PR_TexCoords tex_coords;
} PR_TextureElement;

typedef struct PR_ArrayTexture {
    PR_TextureElement *elements;
    int elements_len;
    unsigned int id;
} PR_ArrayTexture;

typedef struct PR_Font {
    const char* filename;
    unsigned int texture;
    stbtt_bakedchar* char_data;
    int first_char;
    int num_chars;
    int font_height;
    int bitmap_width;
    int bitmap_height;
} PR_Font;

typedef struct PR_Texture {
    unsigned int id;
    int width;
    int height;
    int nr_channels;
} PR_Texture;

typedef struct PR_DataImage {
    uint8_t *data;
    int width;
    int height;
    int nr_channels;
    const char *path;
} PR_DataImage;

typedef struct PR_DataImages {
    PR_DataImage *items;
    size_t count;
    size_t capacity;
} PR_DataImages;

typedef struct PR_Renderer {
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
} PR_Renderer;

PR_TexCoords
texcoords_in_texture_space(size_t x, size_t y, size_t w, size_t h, PR_Texture tex, bool inverse);

void
renderer_init(PR_Renderer *renderer);

// NOTE: Unicolor rendering
void
renderer_add_queue_uni(float x, float y, float w, float h, float r, vec4f c, bool triangle, bool centered);

inline void
renderer_add_queue_uni(PR_Rect rec, vec4f c, bool centered) {
    renderer_add_queue_uni(rec.pos.x, rec.pos.y,
                          rec.dim.x, rec.dim.y, rec.angle,
                          c, rec.triangle, centered);
}

void
renderer_draw_uni(PR_Shader s);


// NOTE: Textured rendering
// This is intended to be used with a single texture containing everything
void
renderer_create_texture(PR_Texture* t, const char* filename);

void
renderer_add_queue_tex(float x, float y, float w, float h, float r, bool centered, float tx, float ty, float tw, float th);

inline void
renderer_add_queue_tex(PR_Rect rec, PR_TexCoords t, bool centered) {
    renderer_add_queue_tex(rec.pos.x, rec.pos.y,
                           rec.dim.x, rec.dim.y,
                           rec.angle, centered,
                           t.tx, t.ty,
                           t.tw, t.th);
}

void
renderer_draw_tex(PR_Shader s, PR_Texture *t);

// NOTE: Texture rendering with array textures

// The ArrayTexture already needs to have elements allocated and elements_len set
//   each element needs to have its filename set aswell
void
renderer_create_array_texture(PR_ArrayTexture *at);

void
renderer_add_queue_array_tex(PR_ArrayTexture at, float x, float y, float w, float h, float r, bool centered, int layer);
void
renderer_draw_array_tex(PR_Shader s, PR_ArrayTexture at);

// NOTE: Text rendering
int
renderer_create_font_atlas(PR_Font *font);

void
renderer_add_queue_text(float x, float y, const char* text, vec4f c, PR_Font *font, bool centered);

void
renderer_draw_text(PR_Font* font, PR_Shader s);

#endif // PR_RENDERER_H
