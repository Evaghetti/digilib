#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

#include "digivice_hal.h"
#include "render.h"
#include "sprites.h"

#define PIXEL_SIZE 20
#define PIXEL_STRIDE 0.9f

static uint8_t screen[16][32];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* background = NULL;

SDL_Texture* loadTexture(const char* path) {
    // SDL_Surface* surface = IMG_Lo
    SDL_Surface* surface = IMG_Load(path);
    if (surface == NULL) {
        SDL_Log("Error loading %s -> %s", path, IMG_GetError());
        return NULL;
    }

    SDL_Texture* texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_FreeSurface(surface);
    if (texture == NULL) {
        SDL_Log("Erro criando textura %s -> %s", path, SDL_GetError());
        return NULL;
    }

    return texture;
}

void renderWindow() {
    int i, j;

    SDL_RenderClear(renderer);
    SDL_RenderCopy(renderer, background, NULL, NULL);

    for (i = 0; i < 16; i++) {
        for (j = 0; j < 32; j++) {
            SDL_Rect rect = {.x = PIXEL_SIZE * j,
                             .y = 70 + PIXEL_SIZE * i ,
                             .w = PIXEL_SIZE * PIXEL_STRIDE,
                             .h = PIXEL_SIZE * PIXEL_STRIDE};
            uint8_t pixelStatus = screen[i][j] ? 255 : 20;

            SDL_Log("%d %d %d %d", rect.x, rect.y, rect.w, rect.h);

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, pixelStatus);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    memset(screen, 0, sizeof(screen));
    SDL_RenderPresent(renderer);
    SDL_Delay(1000 / 60);
}

void setLCDStatus(uint8_t x, uint8_t y, uint8_t uiStatus) {
    screen[y][x] = uiStatus;
}

int main() {

    stDigiviceHal.render = renderWindow;
    stDigiviceHal.setLCDStatus = setLCDStatus;

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        return 1;
    }

    window = SDL_CreateWindow("Digivice", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    if (window == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error init",
                                 SDL_GetError(), NULL);
        return 1;
    }

    renderer = SDL_CreateRenderer(window, -1, 0);
    if (renderer == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error init",
                                 SDL_GetError(), window);
        SDL_DestroyWindow(window);
        return 1;
    }
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    background = loadTexture("resource/background.png");
    if (background == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error init",
                                 "Error loading background", window);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        return 1;
    }

    int run = 1;
    while (run) {
        SDL_Event e;
        while (SDL_PollEvent(&e)) {
            switch (e.type) {
                case SDL_QUIT:
                    run = 0;
                    break;
            }
        }

        DIGIVICE_drawSprite(guiDigimonWalkingAnimationDatabase[2][0], 8, 0, 0);
        stDigiviceHal.render();
    }

    SDL_DestroyTexture(background);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}