#ifndef TEXTURE_H
#define TEXTURE_H

#include "SDL2/SDL_image.h"

SDL_Texture* loadTexture(const char* filePath);

void addRawTexture(SDL_Texture* texture);

void freeTexture(SDL_Texture* texture);

#endif