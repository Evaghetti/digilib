#include "texture.h"

#include "SDL2/SDL.h"

typedef struct LoadedTexture {
    struct LoadedTexture* next;
    SDL_Texture* texture;
    char filePath[256];
} LoadedTexture;

LoadedTexture* headLoadedTexture;

extern SDL_Renderer* gRenderer;

static LoadedTexture* initLoadedTexture(const char* filePath) {
    LoadedTexture* loadedTexture = calloc(1, sizeof(LoadedTexture));
    if (loadedTexture == NULL) {
        SDL_Log("Error while allocating memory for %s", filePath);
        return NULL;
    }

    strncpy(loadedTexture->filePath, filePath, sizeof(loadedTexture->filePath));
    SDL_Surface* surface = IMG_Load(filePath);
    if (surface == NULL) {
        SDL_Log("Error while loading %s from memory", filePath);
        free(loadedTexture);
        return NULL;
    }

    loadedTexture->texture = SDL_CreateTextureFromSurface(gRenderer, surface);
    if (loadedTexture->texture == NULL) {
        SDL_Log("Error while transforming %s into texture", filePath);
        SDL_FreeSurface(surface);
        free(loadedTexture);
        return NULL;
    }

    SDL_Log("Loaded %s!", filePath);
    SDL_FreeSurface(surface);
    loadedTexture->next = NULL;
    return loadedTexture;
}

static SDL_Texture* lookForTexture(LoadedTexture* node, const char* filePath) {
    if (node == NULL) {
        SDL_Log("No node to start looking");
        return NULL;
    }

    while (node != NULL) {
        if (strcmp(node->filePath, filePath) == 0) {
            SDL_Log("%s already loaded! Returning", filePath);
            return node->texture;
        }

        node = node->next;
    }

    SDL_Log("No texture %s has been loaded", filePath);
    return NULL;
}

static LoadedTexture* addTexture(LoadedTexture* node, const char* filePath) {
    if (node == NULL)
        return NULL;

    while (node->next != NULL) {
        node = node->next;
    }

    SDL_Log("Adding new texture");
    node->next = initLoadedTexture(filePath);
    return node->next;
}

SDL_Texture* loadTexture(const char* filePath) {
    if (headLoadedTexture == NULL) {
        SDL_Log("First texture to be loaded");
        headLoadedTexture = initLoadedTexture(filePath);
        return headLoadedTexture->texture;
    }

    SDL_Texture* texture = lookForTexture(headLoadedTexture, filePath);
    if (texture)
        return texture;

    LoadedTexture* addedLoadedTexture = addTexture(headLoadedTexture, filePath);
    if (addedLoadedTexture)
        return addedLoadedTexture->texture;
    return NULL;
}

void freeTexture(SDL_Texture* texture) {
    if (headLoadedTexture == NULL)
        return;

    LoadedTexture *node = headLoadedTexture, *parent = NULL;
    while (node != NULL) {
        if (node->texture == texture) {
            SDL_Log("Destroying %s", node->filePath);
            SDL_DestroyTexture(node->texture);
            if (parent != NULL) {
                SDL_Log("%s has a parent! Putting its next", node->filePath);
                parent->next = node->next;
            }
            SDL_Log("Freeing node");
            free(node);
            return;
        }

        parent = node;
        node = node->next;
    }
}