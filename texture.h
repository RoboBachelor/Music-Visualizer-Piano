#pragma once

#define FREEGLUT_STATIC
#include <iostream>
#include <GL/freeglut.h>

typedef struct {
	uint32_t width;
	uint32_t height;
	uint32_t size;
	uint32_t textureId;
	uint8_t* data;
} texture_t;


int loadBmp(const char* path, texture_t* texture);
void loadTexture(texture_t* texture);
