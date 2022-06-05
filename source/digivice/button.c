#include "button.h"

#include "texture.h"

Button initButton(const char* texturePath, SDL_Rect transform,
                  SDL_Rect spriteClip) {
    Button ret = {.transform = transform,
                  .spriteClip = spriteClip,
                  .hovering = 0,
                  .clicked = 0};
    ret.texture = loadTexture(texturePath);
    return ret;
}

void drawButton(SDL_Renderer* renderer, Button* button) {
    SDL_RenderCopy(renderer, button->texture, &button->spriteClip,
                   &button->transform);
}

void freeButton(Button* button) {
    freeTexture(button->texture);
}