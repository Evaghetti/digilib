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
        RULE_OF_THREE(552, configuration.widthScreen, 912);
    configuration.overlayArea.h =
        RULE_OF_THREE(395, configuration.heightScreen, 661);
    configuration.overlayArea.x = 0;
    configuration.overlayArea.y =
        RULE_OF_THREE(131, configuration.heightScreen, 661);

    configuration.overlayButtons[SELECT].y =
        RULE_OF_THREE(129, configuration.heightScreen, 661);
    configuration.overlayButtons[CONFIRM].y =
        RULE_OF_THREE(289, configuration.heightScreen, 661);
    configuration.overlayButtons[CANCEL].y =
        RULE_OF_THREE(452, configuration.heightScreen, 661);

    for (i = SELECT; i <= CANCEL; i++) {
        configuration.overlayButtons[i].x =
            RULE_OF_THREE(775, configuration.widthScreen, 912);
        configuration.overlayButtons[i].w =
            RULE_OF_THREE(79, configuration.widthScreen, 912);
        configuration.overlayButtons[i].h =
            RULE_OF_THREE(83, configuration.heightScreen, 661);
    }

    configuration.overlayButtons[RESET].x =
        RULE_OF_THREE(725, configuration.widthScreen, 912);
    configuration.overlayButtons[RESET].y =
        RULE_OF_THREE(390, configuration.heightScreen, 661);
    configuration.overlayButtons[RESET].w =
        RULE_OF_THREE(39, configuration.widthScreen, 912);
    configuration.overlayButtons[RESET].h =
        RULE_OF_THREE(39, configuration.heightScreen, 661);

    configuration.widthSprite =
        (((configuration.normalSpriteSize * 10) * configuration.overlayArea.w) /
         MIN_WIDTH_SCREEN);
    configuration.heightSprite =
        (((configuration.normalSpriteSize * 10) * configuration.overlayArea.h) /
         MIN_HEIGHT_SCREEN);

    configuration.widthSmallSprite = configuration.widthSprite / 2.f;
    configuration.heightSmallSprite = configuration.heightSprite / 2.f;

    configuration.widthButton = configuration.overlayArea.w / 4;
    configuration.heightButton = configuration.heightSmallSprite;

    configuration.stepSprite = -(RULE_OF_THREE(1, configuration.widthSprite,
                                               configuration.normalSpriteSize));
    return getConfiguration();
}

const Configuration* getConfiguration() {
    return &configuration;
}