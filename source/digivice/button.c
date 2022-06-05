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

void setButtonHovering(Button* button, SDL_Point position) {
    button->hovering = SDL_PointInRect(&position, &button->transform);
}

void setButtonClicked(Button* button, SDL_Point position) {
    button->clicked = SDL_PointInRect(&position, &button->transform);
}

void drawButton(SDL_Renderer* renderer, Button* button) {
    SDL_RenderCopy(renderer, button->texture, &button->spriteClip,
                   &button->transform);
}

void freeButton(Button* button) {
    freeTexture(button->texture);
}