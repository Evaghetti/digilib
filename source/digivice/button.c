#include "digivice/button.h"

#include "digivice/texture.h"

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
    Uint8 r, g, b;

    SDL_Rect iconTransform = {
        .x = button->transform.x + button->transform.w / 4,
        .w = button->transform.w / 2,
        .y = button->transform.y + button->transform.h / 3,
        .h = button->transform.h / 2};

    SDL_GetTextureColorMod(button->texture, &r, &g, &b);

    if (button->clicked || button->hovering)
        SDL_SetTextureColorMod(button->texture, 0, 0, 0);
    else
        SDL_SetTextureColorMod(button->texture, 50, 50, 50);

    SDL_RenderCopy(renderer, button->texture, &button->spriteClip,
                   &iconTransform);
    SDL_SetTextureColorMod(button->texture, r, g, b);
}

void freeButton(Button* button) {
    freeTexture(button->texture);
}