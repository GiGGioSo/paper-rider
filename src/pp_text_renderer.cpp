#include "pp_text_renderer.h"

#include "../include/glad/glad.h"

#include "pp_globals.h"

#include <cstdio>
#include <cstring>
#include <iostream>

void text_render_init(TextRenderer *text_renderer) {
    glGenVertexArrays(1, &text_renderer->vao);
    glGenBuffers(1, &text_renderer->vbo);

    glBindVertexArray(text_renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, text_renderer->vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 sizeof(float) * 6 * 7 * 1000,
                 NULL, GL_DYNAMIC_DRAW);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE,
                          7 * sizeof(float), 0);
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE,
                          7 * sizeof(float), (void*) (4 * sizeof(float)));

    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    text_renderer->vertex_count = 0;
    text_renderer->bytes_offset = 0;
    text_renderer->fonts = (Font*) malloc(sizeof(Font) * 1);

    Font* f = text_renderer->fonts;
    f->filename = "arial.ttf";
    f->first_char = 32;
    f->num_chars = 96;
    f->font_height = 32.0f;
    f->bitmap_width = 1024;
    f->bitmap_height = 1024;
    f->char_data = (stbtt_bakedchar*) malloc(sizeof(stbtt_bakedchar) *
                                      f->num_chars);
    std::cout << "Ciao1 " << std::endl;
    text_render_create_font_atlas(f);
    std::cout << "Ciao4" << std::endl;
}

void text_render_create_font_atlas(Font* font) {
    std::cout << "Ciao2" << std::endl;
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // disable byte-alignment restrictions

    unsigned char ttf_buffer[1<<20];
    unsigned char temp_bitmap[font->bitmap_width * font->bitmap_height];

    FILE* font_file = fopen(font->filename, "rb");
    fread(ttf_buffer, 1, 1<<20, font_file);
    fclose(font_file);
    std::cout << "Ciao3" << std::endl;

    stbtt_BakeFontBitmap(ttf_buffer, 0,
                         font->font_height, temp_bitmap,
                         font->bitmap_width, font->bitmap_height,
                         font->first_char, font->num_chars, font->char_data);

    glGenTextures(1, &font->texture);
    glBindTexture(GL_TEXTURE_2D, font->texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, font->bitmap_width, font->bitmap_height, 0, GL_RED, GL_UNSIGNED_BYTE, temp_bitmap);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

void text_render_add_queue(float x, float y,
                           const char *text, glm::vec3 c,
                           Font* font) {

    TextRenderer *text_renderer = &glob->text_rend;

    int length = strlen(text);

    float vertices[length * 6][7];

    for(int i = 0; i < length; i++) {
        if (text[i] >= font->first_char &&
            text[i] < font->first_char+font->num_chars) {

            stbtt_aligned_quad q;
            stbtt_GetBakedQuad(font->char_data,
                               font->bitmap_width,
                               font->bitmap_height,
                               text[i] - font->first_char,
                               &x, &y, &q, 1);

            // down left
            vertices[i*6 + 0][0] = q.x0;
            vertices[i*6 + 0][1] = q.y1;
            vertices[i*6 + 0][2] = q.s0;
            vertices[i*6 + 0][3] = q.t1;
            // color
            vertices[i*6 + 0][4] = c.x;
            vertices[i*6 + 0][5] = c.y;
            vertices[i*6 + 0][6] = c.z;

            // top left
            vertices[i*6 + 1][0] = q.x0;
            vertices[i*6 + 1][1] = q.y0;
            vertices[i*6 + 1][2] = q.s0;
            vertices[i*6 + 1][3] = q.t0;
            //color
            vertices[i*6 + 1][4] = c.x;
            vertices[i*6 + 1][5] = c.y;
            vertices[i*6 + 1][6] = c.z;

            // top right
            vertices[i*6 + 2][0] = q.x1;
            vertices[i*6 + 2][1] = q.y0;
            vertices[i*6 + 2][2] = q.s1;
            vertices[i*6 + 2][3] = q.t0;
            //color
            vertices[i*6 + 2][4] = c.x;
            vertices[i*6 + 2][5] = c.y;
            vertices[i*6 + 2][6] = c.z;

            // down left
            vertices[i*6 + 3][0] = q.x0;
            vertices[i*6 + 3][1] = q.y1;
            vertices[i*6 + 3][2] = q.s0;
            vertices[i*6 + 3][3] = q.t1;
            //color
            vertices[i*6 + 3][4] = c.x;
            vertices[i*6 + 3][5] = c.y;
            vertices[i*6 + 3][6] = c.z;

            // top right
            vertices[i*6 + 4][0] = q.x1;
            vertices[i*6 + 4][1] = q.y0;
            vertices[i*6 + 4][2] = q.s1;
            vertices[i*6 + 4][3] = q.t0;
            //color
            vertices[i*6 + 4][4] = c.x;
            vertices[i*6 + 4][5] = c.y;
            vertices[i*6 + 4][6] = c.z;

            // down right
            vertices[i*6 + 5][0] = q.x1;
            vertices[i*6 + 5][1] = q.y1;
            vertices[i*6 + 5][2] = q.s1;
            vertices[i*6 + 5][3] = q.t1;
            //color
            vertices[i*6 + 5][4] = c.x;
            vertices[i*6 + 5][5] = c.y;
            vertices[i*6 + 5][6] = c.z;
        }
    }

    glBindVertexArray(text_renderer->vao);
    glBindBuffer(GL_ARRAY_BUFFER, text_renderer->vbo);

    glBufferSubData(GL_ARRAY_BUFFER, text_renderer->bytes_offset, sizeof(vertices), vertices);

    text_renderer->bytes_offset += sizeof(vertices);

    text_renderer->vertex_count += length * 6;
    


    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);
}

void text_render_draw(Font* font, Shader s) {
    TextRenderer *text_renderer = &glob->text_rend;

    glUseProgram(s);

    shaderer_set_int(s, "text", 0);
    glActiveTexture(GL_TEXTURE0);

    glBindVertexArray(text_renderer->vao);
    glBindTexture(GL_TEXTURE_2D, font->texture);

    glDrawArrays(GL_TRIANGLES, 0, text_renderer->vertex_count);

    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);

    text_renderer->bytes_offset = 0;
    text_renderer->vertex_count = 0;
}

