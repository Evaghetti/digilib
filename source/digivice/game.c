#include "digivice/game.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "animation.h"
#include "texture.h"

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* test = NULL;
AnimationController animationController;

int initGame() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("Digivice", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, 640, 480, 0);
    if (window == NULL) {
        SDL_Log("Failed creating window");
        return 0;
    }

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    if (renderer == NULL) {
        SDL_Log("Failed creating renderer from window");
        cleanUpGame();
        return 0;
    }

    test = loadTexture("resource/metal greymon.gif");
    if (test == NULL) {
        SDL_Log("Failed loading texture");
        cleanUpGame();
        return 0;
    }

    addAnimation(&animationController, "walking", 9, createRect(0, 0, 16, 16),
                 1.f, createRect(16, 0, 16, 16), 1.f, createRect(0, 0, 16, 16),
                 1.f, createRect(16, 0, 16, 16), 1.f, createRect(0, 0, 16, 16),
                 1.f, createRect(16, 0, 16, 16), 1.f, createRect(0, 0, 16, 16),
                 1.f, createRect(16, 0, 16, 16), 1.f, createRect(32, 0, 16, 16),
                 1.f);
    setCurrentAnimation(&animationController, "walking");

    SDL_Log("Initialized");
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
    }

    int nowTime = SDL_GetPerformanceCounter();
    if (lastTime == -1)
        lastTime = nowTime;

    float deltaTime =
        (float)(nowTime - lastTime) / (float)SDL_GetPerformanceFrequency();
    lastTime = nowTime;

    // TODO: Game Logic.
    updateAnimation(&animationController, deltaTime);
    return 1;
}

void drawGame() {
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    int tamanho = 256;

    const SDL_Rect* clip = getAnimationFrameClip(&animationController);
    SDL_Rect position = {640 / 2 - tamanho / 2, 480 / 2 - tamanho / 2, tamanho,
                         tamanho};

    SDL_RenderCopy(renderer, test, clip, &position);

    SDL_RenderPresent(renderer);

    SDL_Delay(1000 / 60);  // 60 fps
}

void cleanUpGame() {
    freeAnimationController(&animationController);

    if (test) {
        SDL_Log("Freeing texture");
        freeTexture(test);
    }

    if (renderer) {
        SDL_Log("Destroying renderer");
        SDL_DestroyRenderer(renderer);
    }

    if (window) {
        SDL_Log("Destroying window");
        SDL_DestroyWindow(window);
    }

    SDL_Log("Qutting SDL");
    SDL_Quit();
}