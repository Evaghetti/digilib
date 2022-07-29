#ifndef BUTTON_H
#define BUTTON_H

#include <SDL_image.h>
#include <SDL_rect.h>
#include <SDL_render.h>

typedef struct Buttton {
    SDL_Rect transform, spriteClip;
    SDL_Texture* texture;

    int hovering, clicked;
} Button;

Button initButton(const char* texturePath, SDL_Rect transform,
                  SDL_Rect spriteClip);

void setButtonHovering(Button* button, SDL_Point position);

void setButtonClicked(Button* button, SDL_Point position);

void drawButton(SDL_Renderer* renderer, Button* button);

void freeButton(Button* button);

#endif