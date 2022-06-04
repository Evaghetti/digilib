#include "game.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include <stdio.h>

#include "digiapi.h"

#include "digivice/avatar.h"
#include "digivice/menu.h"
#include "globals.h"

SDL_Window* window = NULL;
Menu currentMenu;
Avatar digimon;

int initGame() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("Digivice", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WIDTH_SCREEN,
                              HEIGHT_SCREEN, 0);
    if (window == NULL) {
        SDL_Log("Failed creating window");
        return 0;
    }

    gRenderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (gRenderer == NULL) {
        SDL_Log("Failed creating renderer from window");
        cleanUpGame();
        return 0;
    }

    if (TTF_Init() == -1) {
        SDL_Log("Failed to initialize TTF");
        cleanUpGame();
        return 0;
    }

    gFonte = TTF_OpenFont("resource/font.ttf", 12);
    if (gFonte == NULL) {
        SDL_Log("Failed loading font");
        cleanUpGame();
        return 0;
    }

    initAvatar(&digimon);

    SDL_Log("Initialized %d", STEP_SPRITE);
    return 1;
}

static void initiateDigitamaMenu() {
    unsigned char count;
    digimon_t** digitamas = DIGI_possibleDigitama(&count);

    char** fileNames = calloc(count, sizeof(char*));
    SDL_Rect* spriteClips = calloc(count, sizeof(SDL_Rect));

    const SDL_Rect clip = {0, 0, 16, 16};
    const int sizeFileName = sizeof(digitamas[0]->szName) + 10;

    int i, j;
    for (i = 0; i < count; i++) {
        fileNames[i] = calloc(sizeFileName, sizeof(char));
        snprintf(fileNames[i], sizeFileName, "resource/%s.gif",
                 digitamas[i]->szName);
        for (j = 9; j < sizeFileName; j++)
            fileNames[i][j] = tolower(fileNames[i][j]);
        spriteClips[i] = clip;
    }

    currentMenu = initMenuImage(count, fileNames, spriteClips);
    for (i = 0; i < count; i++)
        free(fileNames[i]);
    free(fileNames);
    free(spriteClips);
}

static void handleDigitamaMenu(SDL_Scancode scanCode) {
    if (digimon.initiated)
        return;

    switch (scanCode) {
        case SDL_SCANCODE_LEFT:
            advanceMenu(&currentMenu, -1);
            break;
        case SDL_SCANCODE_RIGHT:
            advanceMenu(&currentMenu, 1);
            break;
        case SDL_SCANCODE_RETURN:
            DIGI_initDigitama(SAVE_FILE, currentMenu.currentOption);
            freeMenu(&currentMenu);
            initAvatar(&digimon);
            break;
    }
}

int updateGame() {
    static int lastTime = -1;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            SDL_Log("Closing game");
            return 0;
        } else if (e.type == SDL_KEYUP) {
            handleDigitamaMenu(e.key.keysym.scancode);
        }
    }

    int nowTime = SDL_GetPerformanceCounter();
    if (lastTime == -1)
        lastTime = nowTime;

    float deltaTime =
        (float)(nowTime - lastTime) / (float)SDL_GetPerformanceFrequency();
    lastTime = nowTime;

    if (!digimon.initiated && !currentMenu.countOptions)
        initiateDigitamaMenu();
    updateAvatar(&digimon, deltaTime);
    return 1;
}

void drawGame() {
    SDL_RenderClear(gRenderer);

    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    if (currentMenu.countOptions)
        drawMenu(gRenderer, &currentMenu);
    else
        drawAvatar(gRenderer, &digimon);

    SDL_RenderPresent(gRenderer);

    SDL_Delay(1000 / 60);  // 60 fps
}

void cleanUpGame() {
    if (gFonte)
        TTF_CloseFont(gFonte);

    if (gRenderer) {
        SDL_Log("Destroying renderer");
        SDL_DestroyRenderer(gRenderer);
    }

    if (window) {
        SDL_Log("Destroying window");
        SDL_DestroyWindow(window);
    }

    SDL_Log("Qutting SDL");
    TTF_Quit();
    SDL_Quit();
}