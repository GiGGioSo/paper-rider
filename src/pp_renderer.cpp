#include "pp_renderer.h"

#include "../include/glad/glad.h"
#include "../include/stb_image.h"

#include "pp_globals.h"
#include "pp_shaderer.h"

#include <cstring>
#include <iostream>
#include <math.h>

#define PR_MAX_UNICOLOR_QUADS 2000

#define PR_MAX_TEXTURED_QUADS 10

#define PR_MAX_TEXT_QUADS 1000

// TODO: Render using a `Rect`, so is more convenient
//          to draw the plane and the obstacles

// TODO: Add alpha support for textured quads

// General setup
void renderer_init(Renderer* renderer) {
    // NOTE: unicolor rendering initialization
    glGenVertexArrays(1, &renderer->uni_vao);
    glGenBuffers(1, &renderer->uni_vbo);

    glBindVertexArray(renderer->uni_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->uni_vbo);

    // NOTE: `PR_MAX_UNICOLOR_QUADS` is the max number of unicolor quads
    //                               displayable together on the screen
    //       6 is the number of vertices for a single quad
    //       6 is the number of floats per vertex (2: position, 4: color)
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 6 * 6 * PR_MAX_UNICOLOR_QUADS,
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
    renderer->uni_vertex_count = 0;
    renderer->uni_bytes_offset = 0;

 
    // NOTE: textured rendering initialization
    glGenVertexArrays(1, &renderer->tex_vao);
    glGenBuffers(1, &renderer->tex_vbo);

    glBindVertexArray(renderer->tex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->tex_vbo);

    // NOTE: `PR_MAX_TEXTURED_QUADS` is the max number of textured quads
    //                               displayable together on the screen
    //       6 is the number of vertices for a single quad
    //       4 is the number of floats per vertex (2: position, 2: tex_coords)
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 6 * 4 * PR_MAX_TEXTURED_QUADS,
                 NULL, GL_DYNAMIC_DRAW);

    // Everything can be done in a single attribute pointer
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4,
                          GL_FLOAT, GL_FALSE,
                          4 * sizeof(float), (void*) 0);
    renderer->tex_vertex_count = 0;
    renderer->tex_bytes_offset = 0;

    // NOTE: text rendering initialization
    glGenVertexArrays(1, &renderer->text_vao);
    glGenBuffers(1, &renderer->text_vbo);

    glBindVertexArray(renderer->text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->text_vbo);

    // NOTE: `PR_MAX_TEXT_QUADS` is the max number of text quads
    //                           displayable together on the screen
    //       6 is the number of vertices for a single quad
    //       8 is the number of floats per vertex:
    //          - 2: position
    //          - 2: tex_coords
    //          - 4: color
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 6 * 8 * PR_MAX_TEXT_QUADS,
                 NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE,
                          8 * sizeof(float), (void*) (4 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    renderer->text_vertex_count = 0;
    renderer->text_bytes_offset = 0;
}

// NON-textured quads
void renderer_add_queue_uni(float x, float y,
                           float w, float h,
                           float r, glm::vec4 c,
                           bool triangle, bool centered) {

    Renderer* renderer = &glob->renderer;

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

    glBindVertexArray(renderer->uni_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->uni_vbo);

    glBufferSubData(GL_ARRAY_BUFFER,
                    renderer->uni_bytes_offset,
                    vertices_number * 6 * sizeof(float),
                    vertices);

    renderer->uni_bytes_offset += vertices_number * 6 * sizeof(float);
    renderer->uni_vertex_count += vertices_number;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderer_draw_uni(Shader s) {
    Renderer* renderer = &glob->renderer;

    glUseProgram(s);

    glBindVertexArray(renderer->uni_vao);

    glDrawArrays(GL_TRIANGLES, 0, renderer->uni_vertex_count);

    glBindVertexArray(0);

    renderer->uni_bytes_offset = 0;
    renderer->uni_vertex_count = 0;
}

// Textured quads
void renderer_create_texture(Texture* t, const char* filepath) {
    stbi_set_flip_vertically_on_load(true);

    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    // texture options
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    unsigned char* data = stbi_load(filepath,
                                    &t->width, &t->height,
                                    &t->nr_channels, 0);
    //Generate texture
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA,
                     t->width, t->height,
                     0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "[ERROR] Failed to load texture: "
                  << filepath
                  << std::endl;
    }
    // remove image data, not needed anymore because it's already in the texture
    stbi_image_free(data);
}

void renderer_add_queue_tex(float x, float y,
                               float w, float h,
                               float r, bool centered,
                               float tx, float ty,
                               float tw, float th) {

    Renderer* renderer = &glob->renderer;

    if (centered) {
        x -= w/2;
        y -= h/2;
    }

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

        float newX = center_x +
                     (vx - center_x) * cos(r) +
                     (vy - center_y) * sin(r);
        float newY = center_y +
                     (vx - center_x) * sin(r) -
                     (vy - center_y) * cos(r);

        vertices[i*4 + 0] = newX;
        vertices[i*4 + 1] = newY;
    }

    glBindVertexArray(renderer->tex_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->tex_vbo);

    glBufferSubData(GL_ARRAY_BUFFER,
                    renderer->tex_bytes_offset,
                    sizeof(vertices), vertices);

    renderer->tex_bytes_offset += sizeof(vertices);
    renderer->tex_vertex_count += 6;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderer_draw_tex(Shader s, Texture* t) {
    Renderer* renderer = &glob->renderer;

    glUseProgram(s);

    shaderer_set_int(s, "tex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, t->id);

    glBindVertexArray(renderer->tex_vao);

    glDrawArrays(GL_TRIANGLES, 0, renderer->tex_vertex_count);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    renderer->tex_bytes_offset = 0;
    renderer->tex_vertex_count = 0;
}

// Text quads
#define return_defer(ret) do { result = ret; goto defer; } while(0)
int renderer_create_font_atlas(Font* font) {
    int result = 0;
    FILE *font_file = NULL;
    uint8_t *ttf_buffer;
    uint8_t *bitmap_buffer;

    {
        // disable byte-alignment restrictions
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        ttf_buffer = (uint8_t *) std::malloc(sizeof(uint8_t) * 1<<20);
        if (ttf_buffer == NULL) return_defer(1);
        bitmap_buffer = (uint8_t *) std::malloc(sizeof(uint8_t) *
                                                font->bitmap_width *
                                                font->bitmap_height);
        if (bitmap_buffer == NULL) return_defer(2);

        font_file = std::fopen(font->filename, "rb");
        if (font_file == NULL) return_defer(3);

        std::fread(ttf_buffer, sizeof(*ttf_buffer), 1<<20, font_file);
        if (std::ferror(font_file)) return_defer(4);

        stbtt_BakeFontBitmap(ttf_buffer, 0,
                             font->font_height, bitmap_buffer,
                             font->bitmap_width, font->bitmap_height,
                             font->first_char, font->num_chars,
                             font->char_data);

        glGenTextures(1, &font->texture);
        glBindTexture(GL_TEXTURE_2D, font->texture);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED,
                     font->bitmap_width, font->bitmap_height,
                     0, GL_RED, GL_UNSIGNED_BYTE, bitmap_buffer);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

    defer:
    if (font_file) std::fclose(font_file);
    if (ttf_buffer) std::free(ttf_buffer);
    if (ttf_buffer) std::free(bitmap_buffer);
    return result;
}

void renderer_add_queue_text(float x, float y,
                             const char *text, glm::vec4 c,
                             Font* font, bool centered) {

    Renderer *renderer = &glob->renderer;

    int length = strlen(text);

    float vertices[length * 6][8];

    float minX = 0.f;
    float minY = 0.f;
    float maxX = 0.f;
    float maxY = 0.f;

    for(int i = 0; i < length; i++) {
        if (text[i] >= font->first_char &&
            text[i] < font->first_char+font->num_chars) {

            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->char_data,
                               font->bitmap_width,
                               font->bitmap_height,
                               text[i] - font->first_char,
                               &x, &y, &q, 1);

            if (i == 0) {
                minX = q.x0;
                minY = q.y0;
                maxX = q.x1;
                maxY = q.y1;
            } else {
                if (q.x0 < minX) minX = q.x0;
                if (q.y0 < minY) minY = q.y0;
                if (q.x1 > maxX) maxX = q.x1;
                if (q.y1 > maxY) maxY = q.y1;
            }

            // down left
            vertices[i*6 + 0][0] = q.x0;
            vertices[i*6 + 0][1] = q.y1;
            vertices[i*6 + 0][2] = q.s0;
            vertices[i*6 + 0][3] = q.t1;
            // color
            vertices[i*6 + 0][4] = c.r;
            vertices[i*6 + 0][5] = c.g;
            vertices[i*6 + 0][6] = c.b;
            vertices[i*6 + 0][7] = c.a;

            // top left
            vertices[i*6 + 1][0] = q.x0;
            vertices[i*6 + 1][1] = q.y0;
            vertices[i*6 + 1][2] = q.s0;
            vertices[i*6 + 1][3] = q.t0;
            //color
            vertices[i*6 + 1][4] = c.r;
            vertices[i*6 + 1][5] = c.g;
            vertices[i*6 + 1][6] = c.b;
            vertices[i*6 + 1][7] = c.a;

            // top right
            vertices[i*6 + 2][0] = q.x1;
            vertices[i*6 + 2][1] = q.y0;
            vertices[i*6 + 2][2] = q.s1;
            vertices[i*6 + 2][3] = q.t0;
            //color
            vertices[i*6 + 2][4] = c.r;
            vertices[i*6 + 2][5] = c.g;
            vertices[i*6 + 2][6] = c.b;
            vertices[i*6 + 2][7] = c.a;

            // down left
            vertices[i*6 + 3][0] = q.x0;
            vertices[i*6 + 3][1] = q.y1;
            vertices[i*6 + 3][2] = q.s0;
            vertices[i*6 + 3][3] = q.t1;
            //color
            vertices[i*6 + 3][4] = c.r;
            vertices[i*6 + 3][5] = c.g;
            vertices[i*6 + 3][6] = c.b;
            vertices[i*6 + 3][7] = c.a;

            // top right
            vertices[i*6 + 4][0] = q.x1;
            vertices[i*6 + 4][1] = q.y0;
            vertices[i*6 + 4][2] = q.s1;
            vertices[i*6 + 4][3] = q.t0;
            //color
            vertices[i*6 + 4][4] = c.r;
            vertices[i*6 + 4][5] = c.g;
            vertices[i*6 + 4][6] = c.b;
            vertices[i*6 + 4][7] = c.a;

            // down right
            vertices[i*6 + 5][0] = q.x1;
            vertices[i*6 + 5][1] = q.y1;
            vertices[i*6 + 5][2] = q.s1;
            vertices[i*6 + 5][3] = q.t1;
            //color
            vertices[i*6 + 5][4] = c.r;
            vertices[i*6 + 5][5] = c.g;
            vertices[i*6 + 5][6] = c.b;
            vertices[i*6 + 5][7] = c.a;
        } else {
            std::cout << "Unexpected char: " << text[i] << std::endl;
            return;
        }
    }

    if (centered) {
        float half_text_w = (maxX - minX) * 0.5f;
        float half_text_h = (maxY - minY) * 0.5f;

        for(size_t vertex_index = 0;
            vertex_index < length * 6;
            ++vertex_index) {

            vertices[vertex_index][0] -= half_text_w;
            vertices[vertex_index][1] += half_text_h;
        }


    }

    glBindVertexArray(renderer->text_vao);
    glBindBuffer(GL_ARRAY_BUFFER, renderer->text_vbo);

    glBufferSubData(GL_ARRAY_BUFFER,
                    renderer->text_bytes_offset,
                    sizeof(vertices), vertices);

    renderer->text_bytes_offset += sizeof(vertices);
    renderer->text_vertex_count += length * 6;

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void renderer_draw_text(Font* font, Shader s) {
    Renderer *renderer = &glob->renderer;

    glUseProgram(s);

    shaderer_set_int(s, "tex", 0);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, font->texture);

    glBindVertexArray(renderer->text_vao);

    glDrawArrays(GL_TRIANGLES, 0, renderer->text_vertex_count);

    glBindTexture(GL_TEXTURE_2D, 0);
    glBindVertexArray(0);

    renderer->text_bytes_offset = 0;
    renderer->text_vertex_count = 0;
}

