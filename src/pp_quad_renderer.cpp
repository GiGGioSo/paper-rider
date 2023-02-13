#include "pp_quad_renderer.h"
#include "../include/glad/glad.h"

#include "pp_globals.h"
#include "pp_shaderer.h"

#include <iostream>
#include <math.h>

#define PP_MAX_UNICOLOR_QUADS 2000

#define PP_MAX_TEXTURED_QUADS 10

// TODO: Render using a `Rect`, so is more convenient
//          to draw the plane and the obstacles

// TODO: Add alpha support for textured quads

void quad_render_init(RendererQuad* quad_renderer) {

    // Initialization of unicolor VAO and VBO
    glGenVertexArrays(1, &quad_renderer->vao);
    glGenBuffers(1, &quad_renderer->vbo);

    glBindVertexArray(quad_renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_renderer->vbo);

    // NOTE: `PP_MAX_UNICOLOR_QUADS` is the max number of unicolor quads displayable together on the screen
    //       6 is the number of vertices for a single quad
    //       6 is the number of floats per vertex (2: position, 4: color)
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 6 * 6 * PP_MAX_UNICOLOR_QUADS,
                 NULL, GL_DYNAMIC_DRAW);

    // Vertex position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 2,
                          GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*) 0);
    // Vertex color
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4,
                          GL_FLOAT, GL_FALSE,
                          6 * sizeof(float), (void*) (2 * sizeof(float)));

 
    // Initialization of textured VAO and VBO
    glGenVertexArrays(1, &quad_renderer->tex_vao);
    glGenBuffers(1, &quad_renderer->tex_vbo);

    glBindVertexArray(quad_renderer->tex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_renderer->tex_vbo);

    // NOTE: 10 is the max number of textured quads displayable together on the screen
    //       6 is the number of vertices for a single quad
    //       4 is the number of floats per vertex (2: position, 3: tex_coords)
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 6 * 4 * PP_MAX_TEXTURED_QUADS,
                 NULL, GL_DYNAMIC_DRAW);

    // Everything can be done in a single attribute pointer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4,
                          GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*) 0);


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    quad_renderer->bytes_offset = 0;
    quad_renderer->vertex_count = 0;

}

void quad_render_add_queue(float x, float y, float w, float h, float r, glm::vec4 c, bool triangle, bool centered) {

    RendererQuad* quad_renderer = &glob->rend.quad_renderer;

    if (centered) {
        x -= w/2;
        y -= h/2;
    }

    // NOTE: All the vertices get prepared, but only the 
    //       necessary amount gets processed.
    size_t vertices_number = triangle ? 3 : 6;
    float vertices[] = {
        x  , y+h, c.r, c.g, c.b, c.a,
        x+w, y+h, c.r, c.g, c.b, c.a,
        x  , y  , c.r, c.g, c.b, c.a,
        x+w, y  , c.r, c.g, c.b, c.a,
        x+w, y+h, c.r, c.g, c.b, c.a,
        x  , y  , c.r, c.g, c.b, c.a,
    };

    r = glm::radians(-r);
    float center_x = x + w/2;
    float center_y = y + h/2;

    for(int i = 0; i < vertices_number; i++) {
        float vx = vertices[i*6 + 0];
        float vy = vertices[i*6 + 1];

        float newX = center_x +
                     (vx - center_x) * cos(r) +
                     (vy - center_y) * sin(r);
        float newY = center_y +
                     (vx - center_x) * sin(r) -
                     (vy - center_y) * cos(r);

        vertices[i*6 + 0] = newX;
        vertices[i*6 + 1] = newY;
    }

    glBindVertexArray(quad_renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_renderer->vbo);

    glBufferSubData(GL_ARRAY_BUFFER,
                    quad_renderer->bytes_offset,
                    vertices_number * 6 * sizeof(float),
                    vertices);

    quad_renderer->bytes_offset += vertices_number * 6 * sizeof(float);
    quad_renderer->vertex_count += vertices_number;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

}

void quad_render_draw(Shader s) {

    RendererQuad* quad_renderer = &glob->rend.quad_renderer;

    glUseProgram(s);

    glBindVertexArray(quad_renderer->vao);

    glDrawArrays(GL_TRIANGLES, 0, quad_renderer->vertex_count);

    glBindVertexArray(0);

    quad_renderer->bytes_offset = 0;
    quad_renderer->vertex_count = 0;
}

void quad_render_add_queue_tex(float x, float y, float w, float h, float r, float tx, float ty, float tw, float th) {

    RendererQuad* quad_renderer = &glob->rend.quad_renderer;

    // NOTE: tx, ty are supposed to be the lower left corner in texture coordinates
    //       tw, th are the width and height, still in texture coordinates
    //       This means that everything has to be 0 <= x <= 1

    float vertices[] = {
        x  , y+h, tx   , ty+th,
        x  , y  , tx   , ty   ,
        x+w, y  , tx+tw, ty   ,
        x  , y+h, tx   , ty+th,
        x+w, y  , tx+tw, ty   ,
        x+w, y+h, tx+tw, ty+th,
    };

    r = glm::radians(-r);
    float center_x = x + w/2;
    float center_y = y + h/2;

    for(int i = 0; i < 6; i++) {
        float vx = vertices[i*4 + 0];
        float vy = vertices[i*4 + 1];

        float newX = center_x + (vx - center_x) * cos(r) + (vy - center_y) * sin(r);
        float newY = center_y + (vx - center_x) * sin(r) - (vy - center_y) * cos(r);

        vertices[i*4 + 0] = newX;
        vertices[i*4 + 1] = newY;
    }

    glBindVertexArray(quad_renderer->tex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, quad_renderer->tex_vbo);

    glBufferSubData(GL_ARRAY_BUFFER, quad_renderer->bytes_offset, sizeof(vertices), vertices);

    quad_renderer->bytes_offset += sizeof(vertices);
    quad_renderer->vertex_count += 6;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void quad_render_draw_tex(Shader s, Texture* t) {

    RendererQuad* quad_renderer = &glob->rend.quad_renderer;

    glUseProgram(s);

    shaderer_set_int(s, "tex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->id);

    glBindVertexArray(quad_renderer->tex_vao);

    glDrawArrays(GL_TRIANGLES, 0, quad_renderer->vertex_count);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    quad_renderer->bytes_offset = 0;
    quad_renderer->vertex_count = 0;

}
