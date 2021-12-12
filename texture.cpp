#define _CRT_SECURE_NO_WARNINGS

#include <cstdio>
#include "texture.h"

int loadBmp(const char* path, texture_t* texture) {
    unsigned char header[54];
    FILE* file = fopen(path, "rb");

    if (file == nullptr) {
        printf("Can not open %s\n", path);
        return -1;
    }

    fread(header, sizeof(unsigned char), 54, file);
    texture->width = *(int*)&header[18];
    texture->height = *(int*)&header[22];
    texture->size = 3 * texture->width * texture->height;

    texture->data = (uint8_t*)malloc(texture->size);
    fread(texture->data, sizeof(uint8_t), texture->size, file);
    fclose(file);

    //for (int i = 0; i < texture->size; i += 3) {
    //    unsigned char tmp = texture->data[i];
    //    texture->data[i] = texture->data[i + 2];
    //    texture->data[i + 2] = tmp;
    //}
    return 0;
}


void loadTexture(texture_t* texture) {
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);  // set pixel storage modes (in the memory)
    glGenTextures(1, &texture->textureId);
    glBindTexture(GL_TEXTURE_2D, texture->textureId);
    glTexImage2D(GL_TEXTURE_2D, 0, 3, texture->width, texture->height, 0, GL_BGR_EXT, GL_UNSIGNED_BYTE, texture->data);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    glTexParameterf(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    free(texture->data);
}
