#include "digivice/globals.h"

#include <SDL_image.h>

#include <stdarg.h>
#include <stdio.h>

#define MIN_WIDTH_SCREEN   480
#define MIN_HEIGHT_SCREEN  320
#define NORMAL_SIZE_SPRITE 16

static Configuration configuration;

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

#define RULE_OF_THREE(x, y, z) (x * y) / z

const Configuration* initConfiguration(int width, int height) {
    int i;

    configuration.widthScreen = width;
    configuration.heightScreen = height;
    // TODO: Maybe add this as parameter?
    configuration.normalSpriteSize = NORMAL_SIZE_SPRITE;
    configuration.normalSmallSpriteSize = configuration.normalSpriteSize / 2;

    configuration.overlayArea.w =
        RULE_OF_THREE(552.f, (float)configuration.widthScreen, 912.f);
    configuration.overlayArea.h =
        RULE_OF_THREE(395.f, (float)configuration.heightScreen, 661.f);
    configuration.overlayArea.x = 0.f;
    configuration.overlayArea.y =
        RULE_OF_THREE(131.f, (float)configuration.heightScreen, 661.f);

    configuration.overlayButtons[SELECT].y =
        RULE_OF_THREE(129.f, (float)configuration.heightScreen, 661.f);
    configuration.overlayButtons[CONFIRM].y =
        RULE_OF_THREE(289.f, (float)configuration.heightScreen, 661.f);
    configuration.overlayButtons[CANCEL].y =
        RULE_OF_THREE(452.f, (float)configuration.heightScreen, 661.f);

    for (i = SELECT; i <= CANCEL; i++) {
        configuration.overlayButtons[i].x =
            RULE_OF_THREE(775.f, (float)configuration.widthScreen, 912.f);
        configuration.overlayButtons[i].w =
            RULE_OF_THREE(79.f, (float)configuration.widthScreen, 912.f);
        configuration.overlayButtons[i].h =
            RULE_OF_THREE(83.f, (float)configuration.heightScreen, 661.f);
    }

    configuration.overlayButtons[RESET].x =
        RULE_OF_THREE(725.f, (float)configuration.widthScreen, 912.f);
    configuration.overlayButtons[RESET].y =
        RULE_OF_THREE(390.f, (float)configuration.heightScreen, 661.f);
    configuration.overlayButtons[RESET].w =
        RULE_OF_THREE(39.f, (float)configuration.widthScreen, 912.f);
    configuration.overlayButtons[RESET].h =
        RULE_OF_THREE(39.f, (float)configuration.heightScreen, 661.f);

    configuration.widthSprite =
        RULE_OF_THREE((float)(configuration.normalSpriteSize * 10),
                      configuration.overlayArea.w, (float)MIN_WIDTH_SCREEN);

    configuration.heightSprite =
        RULE_OF_THREE((float)(configuration.normalSpriteSize * 10),
                      configuration.overlayArea.h, (float)MIN_HEIGHT_SCREEN);

    configuration.widthSmallSprite = configuration.widthSprite / 2.f;
    configuration.heightSmallSprite = configuration.heightSprite / 2.f;

    configuration.widthButton = configuration.overlayArea.w / 4.f;
    configuration.heightButton = configuration.heightSmallSprite;

    configuration.stepSprite = -(RULE_OF_THREE(
        1.f, configuration.widthSprite, (float)configuration.normalSpriteSize));
    return getConfiguration();
}

const Configuration* getConfiguration() {
    return &configuration;
}