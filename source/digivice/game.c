#include "game.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include "digivice/menu.h"
#include "globals.h"

SDL_Window* window = NULL;
Menu menuTeste;

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

    // SDL_Rect spriteRects[] = {{0, 0, 16, 16}, {0, 0, 16, 16}};
    // char* pathsMenu[] = {"resource/digitama botamon.gif",
    //                      "resource/digitama punimon.gif"};
    // menuTeste = initMenuImage(2, pathsMenu, spriteRects);
    char* options[] = {"OP. 1", "OP. 2", "OP. 3", "OP. 4", "OP. 5"};
    menuTeste = initMenuText(5, options);

    SDL_Log("Initialized %d", STEP_SPRITE);
    return 1;
}

int updateGame() {
    static int lastTime = -1;
    SDL_Event e;

    while (SDL_PollEvent(&e)) {
        if (e.type == SDL_QUIT) {
            SDL_Log("Closing game");
            return 0;
        }
        if (e.type == SDL_KEYUP) {
            advanceMenu(&menuTeste,
                        e.key.keysym.scancode == SDL_SCANCODE_LEFT ? -1 : 1);
        }
    }

    int nowTime = SDL_GetPerformanceCounter();
    if (lastTime == -1)
        lastTime = nowTime;

    float deltaTime =
        (float)(nowTime - lastTime) / (float)SDL_GetPerformanceFrequency();
    lastTime = nowTime;

    return 1;
}

void drawGame() {
    SDL_RenderClear(gRenderer);

    SDL_SetRenderDrawColor(gRenderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    drawMenu(gRenderer, &menuTeste);

    SDL_RenderPresent(gRenderer);

    SDL_Delay(1000 / 60);  // 60 fps
}

void cleanUpGame() {
    freeMenu(&menuTeste);

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