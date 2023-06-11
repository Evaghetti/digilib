#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "digihal.h"
#include "digitype.h"
#include "digivice.h"
#include "digivice_hal.h"

#define WINDOW_WIDTH  640
#define WINDOW_HEIGHT 480

#define ICON_WIDTH  (WINDOW_WIDTH / (COUNT_ICONS / 2))
#define ICON_HEIGHT (WINDOW_HEIGHT * 0.25f)
#define ICON_STRIDE 0.5f

#define MATRIX_WIDTH  (WINDOW_WIDTH)
#define MATRIX_HEIGHT (WINDOW_HEIGHT - ICON_HEIGHT * 2)

#define DOT_WIDTH  (MATRIX_WIDTH / 32)
#define DOT_HEIGHT (MATRIX_HEIGHT / 16)
#define DOT_STRIDE 0.9f

static uint8_t screen[16][32];
static uint8_t iconsStates[COUNT_ICONS];

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* background = NULL;
SDL_Texture* iconsTexture = NULL;

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
            SDL_Rect rect = {.x = DOT_WIDTH * j,
                             .y = ICON_HEIGHT + DOT_HEIGHT * i,
                             .w = DOT_WIDTH * DOT_STRIDE,
                             .h = DOT_HEIGHT * DOT_STRIDE};
            uint8_t pixelStatus = screen[i][j] ? 255 : 20;

            SDL_SetRenderDrawColor(renderer, 0, 0, 0, pixelStatus);
            SDL_RenderFillRect(renderer, &rect);
        }
    }

    for (i = 0; i < COUNT_ICONS / 2; i++) {
        SDL_Rect clip = {.x = 16 * i, .y = 0, .w = 16, .h = 16};
        SDL_Rect transform = {
            .x = (i * ICON_WIDTH) + (ICON_WIDTH * (ICON_STRIDE * .5f)),
            .y = ICON_HEIGHT * (ICON_STRIDE * .5f),
            .w = ICON_WIDTH * ICON_STRIDE,
            .h = ICON_HEIGHT * ICON_STRIDE};

        if (iconsStates[i]) {
            SDL_SetTextureColorMod(iconsTexture, 0, 0, 0);
            SDL_SetTextureAlphaMod(iconsTexture, 255);
        } else {
            SDL_SetTextureColorMod(iconsTexture, 50, 50, 50);
            SDL_SetTextureAlphaMod(iconsTexture, 50);
        }
        SDL_RenderCopy(renderer, iconsTexture, &clip, &transform);
    }

    for (; i < COUNT_ICONS; i++) {
        SDL_Rect clip = {.x = 16 * i, .y = 0, .w = 16, .h = 16};
        SDL_Rect transform = {.x = ((i - COUNT_ICONS / 2) * ICON_WIDTH) +
                                   (ICON_WIDTH * (ICON_STRIDE * .5f)),
                              .y = ICON_HEIGHT + MATRIX_HEIGHT +
                                   ICON_HEIGHT * (ICON_STRIDE * .5f),
                              .w = ICON_WIDTH * ICON_STRIDE,
                              .h = ICON_HEIGHT * ICON_STRIDE};

        if (iconsStates[i]) {
            SDL_SetTextureColorMod(iconsTexture, 0, 0, 0);
            SDL_SetTextureAlphaMod(iconsTexture, 255);
        } else {
            SDL_SetTextureColorMod(iconsTexture, 50, 50, 50);
            SDL_SetTextureAlphaMod(iconsTexture, 50);
        }
        SDL_RenderCopy(renderer, iconsTexture, &clip, &transform);
    }

    memset(screen, 0, sizeof(screen));
    SDL_RenderPresent(renderer);
    SDL_Delay(1000 / 60);
}

void setLCDStatus(uint8_t x, uint8_t y, uint8_t uiStatus) {
    screen[y][x] = uiStatus;
}

void setIconStatus(uint8_t uiIndex, uint8_t uiStatus) {
    iconsStates[uiIndex] = uiStatus;
}

int logLine(const char* fmt, ...) {
    char line[500];
    va_list args;
    va_start(args, fmt);
    vsnprintf(line, sizeof(line), fmt, args);
    va_end(args);

    SDL_Log("[DIGILIB] %s", line);
    return 0;
}

size_t readGame(void* pData, size_t size) {
    SDL_RWops* file = SDL_RWFromFile("save.data", "rb");
    if (file == NULL) {
        logLine("Erro lendo arquivo -> %s", SDL_GetError());
        return 0;
    }

    size_t ret = SDL_RWread(file, pData, 1, size);
    SDL_RWclose(file);
    return ret;
}

size_t saveGame(const void* pData, size_t size) {
    SDL_RWops* file = SDL_RWFromFile("save.data", "wb");
    if (file == NULL) {
        logLine("Erro criando arquivo -> %s", SDL_GetError());
        return 0;
    }

    size_t ret = SDL_RWwrite(file, pData, 1, size);
    SDL_RWclose(file);
    return ret;
}

uint8_t getRandomNumber() {
    srand(time(NULL));
    return rand() % 16;
}

int main() {
    digivice_hal_t stDigiviceHal = {
        .render = renderWindow,
        .setLCDStatus = setLCDStatus,
        .setIconStatus = setIconStatus,
        .getTimeStamp = SDL_GetTicks64,  // teste
    };

    digihal_t stHal = {
        .malloc = SDL_malloc,
        .free = SDL_free,
        .log = logLine,
        .readData = readGame,
        .saveData = saveGame,
        .randomNumber = getRandomNumber,
    };

    if (DIGIVICE_init(&stHal, &stDigiviceHal, SDL_GetPerformanceFrequency()) !=
        0) {
        return 1;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING) != 0) {
        return 1;
    }

    window = SDL_CreateWindow("Digivice", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WINDOW_WIDTH,
                              WINDOW_HEIGHT, 0);
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

    iconsTexture = loadTexture("resource/hud.png");
    if (iconsTexture == NULL) {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, "Error init",
                                 "Error loading HUD", window);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_DestroyTexture(iconsTexture);
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
                case SDL_KEYDOWN:
                case SDL_KEYUP:
                    switch (e.key.keysym.scancode) {
                        case SDL_SCANCODE_Z:
                            DIGIVICE_setButtonState(BUTTON_A,
                                                    e.type == SDL_KEYDOWN);
                            break;
                        case SDL_SCANCODE_X:
                            DIGIVICE_setButtonState(BUTTON_B,
                                                    e.type == SDL_KEYDOWN);
                            break;
                        case SDL_SCANCODE_C:
                            DIGIVICE_setButtonState(BUTTON_C,
                                                    e.type == SDL_KEYDOWN);
                            break;
                        case SDL_SCANCODE_R:
                            DIGIVICE_setButtonState(BUTTON_R,
                                                    e.type == SDL_KEYDOWN);
                            break;

                        default:
                            break;
                    }
                    break;
            }
        }

        DIGIVICE_update();
    }

    SDL_DestroyTexture(background);
    SDL_DestroyTexture(iconsTexture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}