#ifndef TEXTURER_H
#define TEXTURER_H

struct Texture {
    unsigned int id;
    int width;
    int height;
    int nr_channels;
};

void texturer_create_texture(Texture* t, const char* filename);

#endif
