#include "digivice/texture.h"

#include <SDL.h>

#include <stdio.h>
#include <string.h>
#include <time.h>

typedef struct LoadedTexture {
    struct LoadedTexture* next;
    SDL_Texture* texture;
    char filePath[256];
    int resourceCount;
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
        SDL_Log("Error while loading %s from memory -> %s", filePath,
                IMG_GetError());
        free(loadedTexture);
        return NULL;
    }

    loadedTexture->texture = SDL_CreateTextureFromSurface(gRenderer, surface);
    if (loadedTexture->texture == NULL) {
        SDL_Log("Error while transforming %s into texture -> %s", filePath,
                IMG_GetError());
        SDL_FreeSurface(surface);
        free(loadedTexture);
        return NULL;
    }

    SDL_Log("Loaded %s!", filePath);
    SDL_FreeSurface(surface);

    loadedTexture->resourceCount = 1;
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
            SDL_Log("%s already loaded! Returning (current count: %d)",
                    filePath, node->resourceCount + 1);
            node->resourceCount++;
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

void addRawTexture(SDL_Texture* texture) {
    LoadedTexture* node = headLoadedTexture;
    if (node == NULL)
        return;

    while (node->next)
        node = node->next;

    node->next = calloc(sizeof(LoadedTexture), 1);
    snprintf(node->next->filePath, sizeof(node->next->filePath),
             "%ld_generated", (long int)time(NULL));
    node->next->texture = texture;
}

void freeTexture(SDL_Texture* texture) {
    if (headLoadedTexture == NULL)
        return;

    LoadedTexture** nodeTree = &headLoadedTexture;
    while (*nodeTree != NULL) {
        LoadedTexture* contentNode = *nodeTree;

        if (contentNode->texture == texture) {
            SDL_Log("Reducing resource %s by one (%d)", contentNode->filePath,
                    contentNode->resourceCount - 1);

            contentNode->resourceCount--;
            if (contentNode->resourceCount == 0) {
                LoadedTexture* nextNode = contentNode->next;

                SDL_Log("Destroying %s", contentNode->filePath);
                SDL_DestroyTexture(contentNode->texture);

                SDL_Log("Freeing node");
                free(contentNode);

                *nodeTree = nextNode;
                break;
            }
        }

        nodeTree = &contentNode->next;
    }
}