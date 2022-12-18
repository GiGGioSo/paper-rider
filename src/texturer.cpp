#include "texturer.h"

#include "../include/stb_image.h"

#include "../include/glad/glad.h"
#include <iostream>

void texturer_create_texture(Texture* t, const char* filepath) {

    stbi_set_flip_vertically_on_load(true);

    glGenTextures(1, &t->id);
    glBindTexture(GL_TEXTURE_2D, t->id);
    // texture options
    // TODO: Have the possibility to set these parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    unsigned char* data = stbi_load(filepath, &t->width, &t->height, &t->nr_channels, 0);
    //Generate texture
    if (data) {
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, t->width, t->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
        glGenerateMipmap(GL_TEXTURE_2D);
    } else {
        std::cout << "[ERROR] Failed to load texture: " << filepath << std::endl;
    }
    // remove image data, not needed anymore because it's already in the texture
    stbi_image_free(data);
}
