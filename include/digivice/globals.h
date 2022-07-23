#ifndef GLOBAL_H
#define GLOBAL_H

#include <SDL_ttf.h>

#define MIN_WIDTH_SCREEN         480
#define MIN_HEIGHT_SCREEN        320
#define WIDTH_SCREEN             640
#define HEIGHT_SCREEN            480
#define NORMAL_SIZE_SPRITE       16
#define NORMAL_SIZE_SMALL_SPRITE (NORMAL_SIZE_SPRITE / 2)

#define WIDTH_SPRITE \
    (((NORMAL_SIZE_SPRITE * 10) * WIDTH_SCREEN) / MIN_WIDTH_SCREEN)
#define HEIGHT_SPRITE \
    (((NORMAL_SIZE_SPRITE * 10) * HEIGHT_SCREEN) / MIN_HEIGHT_SCREEN)

#define WIDTH_SMALL_SPRITE  (WIDTH_SPRITE / 2)
#define HEIGHT_SMALL_SPRITE (HEIGHT_SPRITE / 2)

#define WIDTH_BUTTON  (WIDTH_SCREEN / 4)
#define HEIGHT_BUTTON (HEIGHT_SMALL_SPRITE)

#define STEP_SPRITE -(((1) * WIDTH_SPRITE) / NORMAL_SIZE_SPRITE)

SDL_Renderer* gRenderer;
TTF_Font* gFonte;

SDL_Surface* createTextSurface(SDL_Color color, const char* fmt, ...);
SDL_Texture* createTextTexture(SDL_Color color, const char* fmt, ...);

void saveTextureToFile(const char* file_name, SDL_Texture* texture);

#endif