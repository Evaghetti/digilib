#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>

SDL_Window* window = NULL;
SDL_Renderer* renderer = NULL;

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

int main() {

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

    SDL_Texture* background = loadTexture("resource/background.png");
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

        SDL_RenderClear(renderer);
        SDL_RenderCopy(renderer, background, NULL, NULL);
        SDL_RenderPresent(renderer);
    }

    SDL_DestroyTexture(background);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    return 0;
}