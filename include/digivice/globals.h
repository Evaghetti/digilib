#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL_rect.h>
#include <SDL_ttf.h>

#define GAME_TICK .5f

typedef enum ControlButtonType {
    SELECT,
    CONFIRM,
    CANCEL,
    RESET,
    COUNT_CONTROL_BUTTON_TYPE,
} ControlButtonType;

typedef struct Configuration {
    int widthScreen;
    int heightScreen;

    SDL_FRect overlayArea;
    SDL_FRect overlayButtons[COUNT_CONTROL_BUTTON_TYPE];

    int normalSpriteSize, normalSmallSpriteSize;
    float widthSprite, heightSprite;

    float widthSmallSprite, heightSmallSprite;
    float widthButton, heightButton;

    float stepSprite;
} Configuration;

SDL_Renderer* gRenderer;
TTF_Font* gFonte;

SDL_Surface* createTextSurface(SDL_Color color, const char* fmt, ...);
SDL_Texture* createTextTexture(SDL_Color color, const char* fmt, ...);

void saveTextureToFile(const char* file_name, SDL_Texture* texture);

const Configuration* initConfiguration(int width, int height);
const Configuration* getConfiguration();

#endif