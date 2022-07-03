#include "game.h"

#include "SDL2/SDL.h"
#include "SDL2/SDL_ttf.h"

#include <stdio.h>

#include "digiapi.h"
#include "digihardware.h"

#include "digivice/avatar.h"
#include "digivice/button.h"
#include "digivice/menu.h"
#include "digivice/texture.h"
#include "globals.h"

static const SDL_Rect spritesButtons[] = {
    {0, 8, 16, 16},  {16, 8, 16, 16}, {32, 8, 16, 16}, {48, 8, 16, 16},
    {64, 8, 16, 16}, {80, 8, 16, 16}, {96, 8, 16, 16}};
static const SDL_Rect clipCallStatus = {112, 8, 16, 16};

typedef enum {
    BIRTHING = -2,
    NO_OPERATION,
    INFORMATION,
    FEED,
    TRAIN,
    BATTLE,
    CLEAN_POOP,
    LIGHTS,
    HEAL,
    COUNT_OPERATIONS
} PossibleOperations;

SDL_Window* window = NULL;
SDL_Texture* background;

Menu currentMenu;
Button buttonsOperations[COUNT_OPERATIONS];
Button buttonCallStatus;
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

    gFonte = TTF_OpenFont("resource/font.ttf", 24);
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
    SDL_Rect transformCall = buttonsOperations[COUNT_OPERATIONS - 1].transform;
    transformCall.x += transformCall.w;
    buttonCallStatus =
        initButton("resource/hud.png", transformCall, clipCallStatus);
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

static int handleMenu(SDL_Scancode scanCode) {
    // If there's no menu, don't do anything.
    if (currentMenu.countOptions == 0)
        return -1;

    int i;
    switch (scanCode) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_UP:
            advanceMenu(&currentMenu, -1);
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_DOWN:
            advanceMenu(&currentMenu, 1);
            break;
        case SDL_SCANCODE_RETURN:
            i = currentMenu.currentOption;
            return i;
        case SDL_SCANCODE_ESCAPE:
            return -2;
    }

    return -3;
}

static void updateButtonsHovering(int x, int y) {
    SDL_Point point = {x, y};
    int i;

    if (!digimon.initiated || digimon.infoApi.pstCurrentDigimon->uiStage == 0)
        return;

    for (i = 0; i < COUNT_OPERATIONS; i++) {
        setButtonHovering(&buttonsOperations[i], point);
    }
}

static PossibleOperations updateButtonsClick(int x, int y) {
    SDL_Point point = {x, y};
    int i, indexClickedButton = NO_OPERATION;

    if (!digimon.initiated || digimon.infoApi.pstCurrentDigimon->uiStage == 0 ||
        (digimon.currentAction != WALKING && digimon.currentAction != SLEEPING))
        return indexClickedButton;

    for (i = 0; i < COUNT_OPERATIONS; i++) {
        setButtonClicked(&buttonsOperations[i], point);

        if (buttonsOperations[i].clicked) {
            indexClickedButton = (PossibleOperations)i;
        }
    }

    return indexClickedButton;
}

static PossibleOperations handleOperation(PossibleOperations operation,
                                          int selectedOption) {
    PossibleOperations responseOperation = operation;

    switch (operation) {
        case INFORMATION:
            if (currentMenu.countOptions == 0) {
                currentMenu = createTexturesInfoMenu(&digimon, gRenderer);
            }
            if (selectedOption == -2) {
                freeMenu(&currentMenu);
                responseOperation = NO_OPERATION;
            }
            break;
        case BIRTHING:
            if (currentMenu.countOptions == 0)
                initiateDigitamaMenu();
            else if (selectedOption >= 0) {
                DIGI_initDigitama(SAVE_FILE, selectedOption);
                freeMenu(&currentMenu);
                initAvatar(&digimon);
                responseOperation = NO_OPERATION;
            }
            break;
        case FEED:
            if (currentMenu.countOptions == 0) {
                char* args[] = {"FEED", "VITAMIN"};
                currentMenu = initMenuText(2, args);
            } else if (selectedOption >= 0) {
                setCurrentAction(&digimon,
                                 selectedOption == 0 ? EATING : STRENGTHNING);
                responseOperation = NO_OPERATION;
                freeMenu(&currentMenu);
            } else if (selectedOption == -2) {
                freeMenu(&currentMenu);
                responseOperation = NO_OPERATION;
            }
            break;
        case LIGHTS:
            if (currentMenu.countOptions == 0) {
                char* args[] = {"ON", "OFF"};
                currentMenu = initMenuText(2, args);
            } else if (selectedOption != -1) {
                if (selectedOption >= 0) {
                    setCurrentAction(&digimon,
                                     selectedOption == 0 ? WALKING : SLEEPING);
                }

                if (selectedOption != -3) {
                    freeMenu(&currentMenu);
                    responseOperation = NO_OPERATION;
                }
            }
            break;
        case HEAL:
            setCurrentAction(&digimon, (digimon.infoApi.uiStats &
                                        (MASK_SICK | MASK_INJURIED))
                                           ? HEALING
                                           : NEGATING);
            responseOperation = NO_OPERATION;
            break;
        case CLEAN_POOP:
            setCurrentAction(&digimon, CLEANING);
            responseOperation = NO_OPERATION;
            break;
        default:
            responseOperation = NO_OPERATION;
            break;
    }

    return responseOperation;
}

int updateGame() {
    static int lastTime = -1;
    static PossibleOperations currentOperation = NO_OPERATION;

    SDL_Event e;
    int selectedOptionMenu = -1, i;

    for (i = 0; i < COUNT_OPERATIONS; i++)
        buttonsOperations[i].clicked = 0;

    if (currentOperation > NO_OPERATION)
        buttonsOperations[currentOperation].clicked = 1;
    buttonCallStatus.clicked = digimon.calling != 0;

    while (SDL_PollEvent(&e)) {
        switch (e.type) {
            case SDL_QUIT:
                SDL_Log("Closing game");
                return 0;
            case SDL_KEYUP:
                selectedOptionMenu = handleMenu(e.key.keysym.scancode);
                break;
            case SDL_MOUSEMOTION:
                updateButtonsHovering(e.motion.x, e.motion.y);
                break;
            case SDL_MOUSEBUTTONUP:
                if (e.button.button == SDL_BUTTON_LEFT &&
                    currentOperation == NO_OPERATION)
                    currentOperation =
                        updateButtonsClick(e.button.x, e.button.y);
                break;
        }
    }

    if (!digimon.initiated)
        currentOperation = BIRTHING;

    currentOperation = handleOperation(currentOperation, selectedOptionMenu);

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
    drawButton(gRenderer, &buttonCallStatus);

    SDL_RenderPresent(gRenderer);

    SDL_Delay(1000 / 60);  // 60 fps
}

void cleanUpGame() {
    DIGIHW_saveDigimon(SAVE_FILE, &digimon.infoApi);

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