#include "digivice/game.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_image.h"
#include "animation.h"
#include "texture.h"

#define MIN_WIDTH_SCREEN   480
#define MIN_HEIGHT_SCREEN  320
#define WIDTH_SCREEN       640
#define HEIGHT_SCREEN      480
#define NORMAL_SIZE_SPRITE 16

#define WIDTH_SPRITE \
    (((NORMAL_SIZE_SPRITE * 12) * WIDTH_SCREEN) / MIN_WIDTH_SCREEN)
#define HEIGHT_SPRITE \
    (((NORMAL_SIZE_SPRITE * 12) * HEIGHT_SCREEN) / MIN_HEIGHT_SCREEN)
#define STEP_SPRITE -(((1) * WIDTH_SPRITE) / NORMAL_SIZE_SPRITE)

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;
SDL_Texture* test = NULL;
AnimationController animationController;
int posX = WIDTH_SCREEN / 2 - WIDTH_SPRITE / 2;

int initGame() {
    SDL_Init(SDL_INIT_EVERYTHING);
    window = SDL_CreateWindow("Digivice", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, WIDTH_SCREEN,
                              HEIGHT_SCREEN, 0);
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

    addAnimation(
        &animationController, "walking", 9, createRect(0, 0, 16, 16), 1.f,
        createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                   NORMAL_SIZE_SPRITE),
        1.f, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE), 1.f,
        createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                   NORMAL_SIZE_SPRITE),
        1.f, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE), 1.f,
        createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                   NORMAL_SIZE_SPRITE),
        1.f, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE), 1.f,
        createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                   NORMAL_SIZE_SPRITE),
        1.f,
        createRect(NORMAL_SIZE_SPRITE * 2, 0, NORMAL_SIZE_SPRITE,
                   NORMAL_SIZE_SPRITE),
        1.f);
    setCurrentAnimation(&animationController, "walking");

    SDL_Log("Initialized %d", STEP_SPRITE);
    return 1;
}

int updateGame() {
    static int lastTime = -1;
    static float contador = 0.f;
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

    contador += deltaTime;
    if (contador > 1.f) {
        posX += STEP_SPRITE;
        contador = 0.f;
    }

    // TODO: Game Logic.
    updateAnimation(&animationController, deltaTime);
    return 1;
}

void drawGame() {
    SDL_RenderClear(renderer);

    SDL_SetRenderDrawColor(renderer, 255, 255, 255, SDL_ALPHA_OPAQUE);

    const SDL_Rect* clip = getAnimationFrameClip(&animationController);
    SDL_Rect position = {posX, HEIGHT_SCREEN / 2 - HEIGHT_SPRITE / 2,
                         WIDTH_SPRITE, HEIGHT_SPRITE};

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