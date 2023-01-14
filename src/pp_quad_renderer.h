#ifndef QUAD_RENDERER_H
#define QUAD_RENDERER_H

#include "../include/glm/glm.hpp"
#include "pp_rect.h"
#include "pp_shaderer.h"
#include "pp_texturer.h"

struct RendererQuad {
    unsigned int vao;
    unsigned int vbo;

    unsigned int tex_vao;
    unsigned int tex_vbo;

    unsigned int bytes_offset;
    unsigned int vertex_count;
};

void quad_render_init(RendererQuad* quad_renderer);

void quad_render_add_queue(float x, float y, float w, float h, float r, glm::vec4 c, bool centered);
void quad_render_draw(Shader s);

// NOTE: This is intended to be used with a single texture containing everything
void quad_render_add_queue_tex(float x, float y, float w, float h, float r, float tx, float ty, float tw, float th);
void quad_render_draw_tex(Shader s, Texture* t);

// NOTE: To increase the ease of use with Rect types
inline
void quad_render_add_queue(Rect rec, glm::vec4 c, bool centered) {
    quad_render_add_queue(rec.pos.x, rec.pos.y,
                          rec.dim.x, rec.dim.y, rec.angle,
                          c, centered);
}

inline
void quad_render_add_queue_tex(Rect rec, TexCoords t) {
    quad_render_add_queue_tex(rec.pos.x, rec.pos.y,
                              rec.dim.x, rec.dim.y, rec.angle,
                              t.tx, t.ty,
                              t.tw, t.th);
}

#endif
