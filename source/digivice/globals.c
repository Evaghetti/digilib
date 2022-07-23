#include "globals.h"

#include <SDL_image.h>

#include <stdarg.h>
#include <stdio.h>

SDL_Surface* createTextSurface(SDL_Color color, const char* fmt, ...) {
    char formattedText[50];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(formattedText, sizeof(formattedText), fmt, vl);
    va_end(vl);

    SDL_Surface* surface = TTF_RenderText_Solid(gFonte, formattedText, color);
    if (surface == NULL) {
        SDL_Log("Error creating surface for message %s", fmt);
        return NULL;
    }

    return surface;
}

SDL_Texture* createTextTexture(SDL_Color color, const char* fmt, ...) {
    char formattedText[50];
    va_list vl;
    va_start(vl, fmt);
    vsnprintf(formattedText, sizeof(formattedText), fmt, vl);
    va_end(vl);

    SDL_Surface* surface = createTextSurface(color, "%s", formattedText);
    if (surface == NULL) {
        va_end(vl);
        SDL_Log("Error creating surface for message %s", fmt);
        return NULL;
    }

    SDL_Texture* result = SDL_CreateTextureFromSurface(gRenderer, surface);
    if (result == NULL) {
        SDL_Log("Error creating texture for message %s", fmt);
        SDL_FreeSurface(surface);
        return NULL;
    }

    SDL_FreeSurface(surface);
    return result;
}

void saveTextureToFile(const char* file_name, SDL_Texture* texture) {
    if (SDL_SetRenderTarget(gRenderer, texture)) {
        SDL_Log("Erro ao salvar %s -> %s", file_name, SDL_GetError());
        return;
    }
    int width, height;

    SDL_QueryTexture(texture, NULL, NULL, &width, &height);
    SDL_Surface* surface =
        SDL_CreateRGBSurface(0, width, height, 32, 0, 0, 0, 0);
    SDL_RenderReadPixels(gRenderer, NULL, surface->format->format,
                         surface->pixels, surface->pitch);
    IMG_SavePNG(surface, file_name);
    SDL_FreeSurface(surface);
}