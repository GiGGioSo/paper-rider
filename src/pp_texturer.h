#ifndef PP_TEXTURER_H
#define PP_TEXTURER_H

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

void texturer_create_texture(Texture* t, const char* filename);

#endif
