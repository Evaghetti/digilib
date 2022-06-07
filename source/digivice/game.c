#include "game.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include <stdio.h>

#include "digiapi.h"

#include "digivice/avatar.h"
#include "digivice/button.h"
#include "digivice/menu.h"
#include "digivice/texture.h"
#include "globals.h"

static const SDL_Rect spritesButtons[] = {
    {0, 8, 16, 16},  {16, 8, 16, 16}, {32, 8, 16, 16}, {48, 8, 16, 16},
    {64, 8, 16, 16}, {80, 8, 16, 16}, {96, 8, 16, 16}};

#define COUNT_OPERATIONS sizeof(spritesButtons) / sizeof(spritesButtons[0])

SDL_Window* window = NULL;
SDL_Texture* background;

Menu currentMenu;
Button buttonsOperations[COUNT_OPERATIONS];
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

    background = loadTexture("resource/background.png");
    initAvatar(&digimon);

    int i;
    for (i = 0; i < 4; i++) {
        SDL_Rect transform = {.x = WIDTH_BUTTON * i,
                              .y = 0,
                              .w = WIDTH_BUTTON,
                              .h = HEIGHT_BUTTON};
        buttonsOperations[i] =
            initButton("resource/hud.png", transform, spritesButtons[i]);
    }

    for (; i < COUNT_OPERATIONS; i++) {
        SDL_Rect transform = {.x = WIDTH_BUTTON * (i - 4),
                              .y = HEIGHT_SCREEN - HEIGHT_BUTTON,
                              .w = WIDTH_BUTTON,
                              .h = HEIGHT_BUTTON};
        buttonsOperations[i] =
            initButton("resource/hud.png", transform, spritesButtons[i]);
    }
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

static int handleMenu(SDL_Scancode scanCode) {
    int buttonClicked = -1, i;
    for (i = 0; i < COUNT_OPERATIONS; i++) {
        if (buttonsOperations[i].clicked) {
            buttonClicked = i;
            break;
        }
    }

    // If there's no current operation.
    if (buttonClicked == -1) {
        SDL_Log("No operation to be processed");
        return -1;
    }

    switch (scanCode) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_UP:
            advanceMenu(&currentMenu, -1);
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_DOWN:
            advanceMenu(&currentMenu, -1);
            break;
        case SDL_SCANCODE_RETURN:
            buttonsOperations[i].clicked = 0;
            i = currentMenu.currentOption;
            freeMenu(&currentMenu);
            return i;
        case SDL_SCANCODE_ESCAPE:
            buttonsOperations[i].clicked = 0;
            freeMenu(&currentMenu);
            return -2;
    }
}

static void updateButtonsHovering(int x, int y) {
    SDL_Point point = {x, y};
    int i;

    if (!digimon.initiated || digimon.infoApi.pstCurrentDigimon->uiStage == 0)
        return;

    for (i = 0; i < COUNT_OPERATIONS; i++) {
        setButtonHovering(&buttonsOperations[i], point);

        if (buttonsOperations[i].hovering)
            SDL_Log("Hovering over button %d", i);
    }
}

static void updateButtonsClick(int x, int y) {
    SDL_Point point = {x, y};
    int i;

    if (!digimon.initiated || digimon.infoApi.pstCurrentDigimon->uiStage == 0)
        return;

    for (i = 0; i < COUNT_OPERATIONS; i++) {
        setButtonClicked(&buttonsOperations[i], point);

        if (buttonsOperations[i].clicked)
            SDL_Log("Clicked on button %d", i);
    }
}

int updateGame() {
    static int lastTime = -1;
    SDL_Event e;
    int i;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                SDL_Log("Closing game");
                return 0;
            case SDL_KEYUP:
                handleDigitamaMenu(e.key.keysym.scancode);
                handleMenu(e.key.keysym.scancode);
                break;
            case SDL_MOUSEMOTION:
                updateButtonsHovering(e.motion.x, e.motion.y);
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT)
                    updateButtonsClick(e.button.x, e.button.y);
                break;
        }
    }

    // TODO: Enums for each button
    if (currentMenu.countOptions == 0) {
        if (!digimon.initiated)
            initiateDigitamaMenu();
        else if (digimon.infoApi.pstCurrentDigimon->uiStage > 0) {
            if (buttonsOperations[1].clicked) {
                char* ops[] = {"FOOD", "VITAMIN"};
                currentMenu = initMenuText(sizeof(ops) / sizeof(ops[0]), ops);
            }
        }
    }

    int nowTime = SDL_GetPerformanceCounter();
    if (lastTime == -1)
        lastTime = nowTime;

    float deltaTime =
        (float)(nowTime - lastTime) / (float)SDL_GetPerformanceFrequency();
    lastTime = nowTime;

    updateAvatar(&digimon, deltaTime);
    return 1;
}

void drawGame() {
    SDL_RenderClear(gRenderer);

    SDL_RenderCopy(gRenderer, background, NULL, NULL);

    if (currentMenu.countOptions)
        drawMenu(gRenderer, &currentMenu);
    else
        drawAvatar(gRenderer, &digimon);

    int i;
    for (i = 0; i < COUNT_OPERATIONS; i++)
        drawButton(gRenderer, &buttonsOperations[i]);

    SDL_RenderPresent(gRenderer);

    SDL_Delay(1000 / 60);  // 60 fps
}

void cleanUpGame() {
    freeAvatar(&digimon);
    freeTexture(background);

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