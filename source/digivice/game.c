#include "digivice/game.h"

#include <SDL.h>
#include <SDL_net.h>
#include <SDL_system.h>
#include <SDL_ttf.h>

#include <ctype.h>
#include <stdio.h>

#include "digiapi.h"
#include "digihardware.h"

#include "digivice/avatar.h"
#include "digivice/battle.h"
#include "digivice/button.h"
#include "digivice/globals.h"
#include "digivice/menu.h"
#include "digivice/texture.h"
#include "digivice/w0rld.h"

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
    SELECTING_BATTLE_MODE,
    CLEAN_POOP,
    LIGHTS,
    HEAL,
    COUNT_OPERATIONS,
    FEED_WAITING,
    BATTLE_ONLINE,
    BATTLE_SINGLE,
    BATTLE_W0RLD,
} PossibleOperations;

SDL_Window* window = NULL;
SDL_Texture *overlay, *background, *popup;

Menu currentMenu;
Button buttonsOperations[COUNT_OPERATIONS];
Button buttonsControl[COUNT_CONTROL_BUTTON_TYPE];
Button buttonCallStatus;
Avatar digimon;

static const Configuration* config;
static char saveFile[100];
static int currentHoveringButton = -1;
static ControlButtonType clickedControlButton = COUNT_CONTROL_BUTTON_TYPE;
static PossibleOperations responseOperation = NO_OPERATION;

static void setFileName() {
    if (strlen(saveFile))
        return;

    char* path = SDL_GetPrefPath("digilib", "digivice");

    snprintf(saveFile, sizeof(saveFile), "%sdigimon.save", path);
    free(path);

    SDL_Log("Save game will be at %s", saveFile);
}

int initGame() {
    SDL_Init(SDL_INIT_EVERYTHING);

    // Default window to 640x480
    SDL_DisplayMode display = {.w = 480, .h = 320};
    Uint32 flags = 0;

#ifdef _USE_DISPLAY_MODE_
    if (SDL_GetDisplayMode(0, 0, &display) != 0) {
        SDL_Log("Impossible to get display resolution -> %s", SDL_GetError());
        return 0;
    }
#endif

#ifdef _ANDROID_BUILD_
    SDL_SetHint(SDL_HINT_ORIENTATIONS, "LandscapeRight|LandscapeLeft");
    flags = SDL_WINDOW_FULLSCREEN | SDL_WINDOW_BORDERLESS;
    int tempWidth = display.w;
    display.w = display.h;
    display.h = tempWidth;
#endif

    setFileName();

    config = initConfiguration(display.w, display.h);

    window = SDL_CreateWindow("Digivice", SDL_WINDOWPOS_CENTERED,
                              SDL_WINDOWPOS_CENTERED, config->widthScreen,
                              config->heightScreen, flags);
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

    if (SDLNet_Init() != 0) {
        SDL_Log("Error initializing SDL_net -> %s", SDLNet_GetError());
        return 0;
    }

    background = loadTexture("resource/background.png");
    overlay = loadTexture("resource/overlay.png");
    popup = loadTexture("resource/popups.gif");
    initAvatar(&digimon, saveFile);

    int i;
    for (i = 0; i < 4; i++) {
        SDL_FRect transform = {.x = config->widthButton * i,
                               .y = config->overlayArea.y,
                               .w = config->widthButton,
                               .h = config->heightButton};
        buttonsOperations[i] =
            initButton("resource/hud.png", transform, spritesButtons[i]);
    }

    for (; i < COUNT_OPERATIONS; i++) {
        SDL_FRect transform = {.x = config->widthButton * (i - 4),
                               .y = config->overlayArea.y +
                                    config->overlayArea.h -
                                    config->heightButton,
                               .w = config->widthButton,
                               .h = config->heightButton};
        buttonsOperations[i] =
            initButton("resource/hud.png", transform, spritesButtons[i]);
    }
    for (i = 0; i < COUNT_CONTROL_BUTTON_TYPE; i++) {
        SDL_Rect transform = {0, 0, 0, 0};

        buttonsControl[i] = initButton("resource/hud.png",
                                       config->overlayButtons[i], transform);
    }
    SDL_FRect transformCall = buttonsOperations[COUNT_OPERATIONS - 1].transform;
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
        return -2;

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
        default:
            break;
    }

    return -3;
}

static void updateButtonsHovering(int x, int y) {
    SDL_FPoint point = {x, y};
    int i;

    if (!digimon.initiated || digimon.infoApi.pstCurrentDigimon->uiStage == 0 ||
        !SDL_PointInFRect(&point, &config->overlayArea))
        return;

    for (i = 0; i < COUNT_OPERATIONS; i++) {
        setButtonHovering(&buttonsOperations[i], point);
        if (buttonsOperations[i].hovering) {
            currentHoveringButton = i;
        }
    }
}

static ControlButtonType updateControlsButtonsClick(int x, int y,
                                                    int* forcedScancode) {
    SDL_FPoint point = {x, y};
    int i;

    for (i = 0; i < COUNT_CONTROL_BUTTON_TYPE; i++) {
        setButtonClicked(&buttonsControl[i], point);

        if (buttonsControl[i].clicked) {
            clickedControlButton = i;
            break;
        }
    }

    const int isTraining =
        digimon.currentAction & (TRAINING | HAPPY | MAD | SHOWING_SCORE);
    switch (clickedControlButton) {
        case SELECT:
            if (currentMenu.countOptions == 0 && !isTraining) {
                if (currentHoveringButton >= 0)
                    buttonsOperations[currentHoveringButton].hovering = 0;
                currentHoveringButton =
                    (currentHoveringButton + 1) % COUNT_OPERATIONS;
                buttonsOperations[currentHoveringButton].hovering = 1;
            }

            if (isTraining)
                *forcedScancode = SDL_SCANCODE_UP;
            else
                *forcedScancode = SDL_SCANCODE_RIGHT;
            break;
        case CONFIRM:
            if (currentHoveringButton >= 0)
                buttonsOperations[currentHoveringButton].clicked = 1;

            if (isTraining)
                *forcedScancode = SDL_SCANCODE_DOWN;
            else
                *forcedScancode = SDL_SCANCODE_RETURN;
            return currentHoveringButton;
        case CANCEL:
            if (currentHoveringButton >= 0)
                buttonsOperations[currentHoveringButton].hovering = 0;
            currentHoveringButton = -1;
            *forcedScancode = SDL_SCANCODE_ESCAPE;
            break;
        case RESET:
            freeAvatar(&digimon);
            remove(saveFile);
            initAvatar(&digimon, saveFile);
            break;
        default:
            clickedControlButton = COUNT_CONTROL_BUTTON_TYPE;
            break;
    }

    return NO_OPERATION;
}

static PossibleOperations updateButtonsClick(int x, int y) {
    SDL_FPoint point = {x, y};
    int i, indexClickedButton = NO_OPERATION;

    if (!digimon.initiated || digimon.infoApi.pstCurrentDigimon->uiStage == 0 ||
        (digimon.currentAction != WALKING &&
         !(digimon.currentAction & (SLEEPING | SICK))))
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
    static int previousOption = -1;
    responseOperation = operation;
    int hasSkipped = selectedOption == -2 || clickedControlButton < RESET;

    switch (operation) {
        case INFORMATION:
            if (currentMenu.countOptions == 0) {
                currentMenu = createTexturesInfoMenu(&digimon, gRenderer);
            } else if (selectedOption == -2) {
                freeMenu(&currentMenu);
                responseOperation = NO_OPERATION;
            }
            break;
        case BIRTHING:
            if (currentMenu.countOptions == 0)
                initiateDigitamaMenu();
            else if (selectedOption >= 0) {
                DIGI_initDigitama(saveFile, selectedOption);
                freeMenu(&currentMenu);
                initAvatar(&digimon, saveFile);
                responseOperation = NO_OPERATION;
            }
            break;
        case FEED:
            if (currentMenu.countOptions == 0) {
                char* args[] = {"FEED", "VITAMIN"};
                currentMenu = initMenuText(2, args);
                if (previousOption >= 0) {
                    currentMenu.currentOption = previousOption;
                    previousOption = -1;
                }
            } else if (selectedOption >= 0) {
                setCurrentAction(&digimon,
                                 selectedOption == 0 ? EATING : STRENGTHNING);
                freeMenu(&currentMenu);
                responseOperation = FEED_WAITING;
                previousOption = selectedOption;
            } else if (selectedOption == -2) {
                freeMenu(&currentMenu);
                responseOperation = NO_OPERATION;
                previousOption = -1;
            }
            break;
        case FEED_WAITING:
            if (finishedCurrentAnimation(&digimon.animationController) ||
                hasSkipped) {
                if (hasSkipped) {
                    markAnimationAsFinished(&digimon.animationController);
                    setCurrentAction(&digimon, WALKING);
                }
                responseOperation = FEED;
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
        case TRAIN:
            if ((digimon.currentAction &
                 (TRAINING | HAPPY | MAD | SHOWING_SCORE)) == 0)
                setCurrentAction(&digimon, TRAINING);
            else {
                if (selectedOption == SDL_SCANCODE_ESCAPE) {
                    setCurrentAction(&digimon, WALKING);
                    responseOperation = NO_OPERATION;
                    break;
                }

                const int isSkippable = (digimon.currentAction & ~TRAINING);
                if (isSkippable && hasSkipped) {
                    markAnimationAsFinished(&digimon.animationController);
                    digimon.timePassed = GAME_TICK;
                    break;
                }

                if (selectedOption == SDL_SCANCODE_UP)
                    setCurrentAction(&digimon, TRAINING_UP);
                else if (selectedOption == SDL_SCANCODE_DOWN)
                    setCurrentAction(&digimon, TRAINING_DOWN);
            }
            break;
        case SELECTING_BATTLE_MODE:
            if (!cannBattle()) {
                setCurrentAction(&digimon, NEGATING);
                responseOperation = NO_OPERATION;
                break;
            }

            if (currentMenu.countOptions == 0) {
                char* options[] = {"SINGLEPLR", "MULTIPLR", "W0RLD"};
                currentMenu = initMenuText(3, options);
            } else if (selectedOption >= 0) {
                switch (currentMenu.currentOption) {
                    case 0:
                        responseOperation = BATTLE_SINGLE;
                        break;
                    case 1:
                        responseOperation = BATTLE_ONLINE;
                        break;
                    case 2:
                        responseOperation = BATTLE_W0RLD;
                        break;
                    default:
                        responseOperation = NO_OPERATION;
                        break;
                }

                freeMenu(&currentMenu);
            } else if (selectedOption == -2) {
                freeMenu(&currentMenu);
                responseOperation = NO_OPERATION;
            }

            break;
        case BATTLE_SINGLE: {
            int result = updateSingleBattle(digimon.infoApi.pstCurrentDigimon,
                                            &currentMenu, selectedOption);
            switch (result) {
                case 1:
                case 2:
                    setBattleAction(&digimon, result,
                                    getChallengedUserTexture());
                    resetPlayers();
                    responseOperation = NO_OPERATION;
                    break;
                case 0:
                    responseOperation = NO_OPERATION;
                    break;
            }
        } break;
        case BATTLE_ONLINE:
            switch (connectToServer(digimon.infoApi.pstCurrentDigimon)) {
                case 0:
                    SDL_Log("Not able to connect to server");
                    setCurrentAction(&digimon, NEGATING);
                    responseOperation = NO_OPERATION;
                    break;
                case 1:
                    registerUser(digimon.infoApi.pstCurrentDigimon);
                    break;
                default:
                    break;
            }

            StatusUpdate status =
                updateClient(&currentMenu, selectedOption, gRenderer);
            int battled = (status & (WIN | LOSE));
            if (selectedOption == -2 || battled) {
                if (currentMenu.options)
                    freeMenu(&currentMenu);

                disconnectFromServer();
                if (battled)
                    setBattleAction(&digimon, status,
                                    getChallengedUserTexture());
                responseOperation = NO_OPERATION;
            } else if (selectedOption >= 0 && status == NOTHING_HAPPENED) {
                if (!challengeUser(selectedOption)) {
                    SDL_Log("Not possible to challenge this user");
                }
            }
            break;
        case BATTLE_W0RLD: {
            if (selectedOption == -2) {
                releaseDCOMLogic();
                responseOperation = NO_OPERATION;
                break;
            }

            int result = doBattleWithDCOM();
            switch (result) {
                case 5:
                case 4:
                    responseOperation = NO_OPERATION;
                    releaseDCOMLogic();
                    // Fallthrough
                case 3:
                    setCurrentAction(&digimon, NEGATING);
                    break;
                case 0:
                    break;
                default:
                    setBattleAction(&digimon, result, NULL);
                    releaseDCOMLogic();
                    responseOperation = NO_OPERATION;
                    break;
            }
        } break;
        default:
            responseOperation = NO_OPERATION;
            break;
    }

    return responseOperation;
}

static float getDeltaTime() {
    static int lastTime = -1;
    int nowTime = SDL_GetPerformanceCounter();
    if (lastTime == -1)
        lastTime = nowTime;

    float deltaTime =
        (float)(nowTime - lastTime) / (float)SDL_GetPerformanceFrequency();
    lastTime = nowTime;

    return deltaTime;
}

int updateGame() {
    static PossibleOperations currentOperation = NO_OPERATION;

    SDL_Event e;
    int selectedOptionMenu = -1, forcedScanCode = -1, i;

    for (i = 0; i < COUNT_OPERATIONS; i++)
        buttonsOperations[i].clicked = 0;

    if (currentOperation > NO_OPERATION)
        buttonsOperations[currentOperation].clicked = 1;
    buttonCallStatus.clicked = digimon.calling != 0;

    clickedControlButton = COUNT_CONTROL_BUTTON_TYPE;
    while (SDL_PollEvent(&e) || forcedScanCode != -1) {
        if (forcedScanCode != -1) {
            e.type = SDL_KEYUP;
            e.key.keysym.scancode = forcedScanCode;
            forcedScanCode = -1;
        }

        switch (e.type) {
            case SDL_QUIT:
                SDL_Log("Closing game");
                return 0;
            case SDL_KEYUP:
                if ((digimon.currentAction & TRAINING) == 0)
                    selectedOptionMenu = handleMenu(e.key.keysym.scancode);
                else
                    selectedOptionMenu = e.key.keysym.scancode;
                break;
            case SDL_WINDOWEVENT:
                if (e.window.event == SDL_WINDOWEVENT_SIZE_CHANGED) {
                    config = initConfiguration(e.window.data1, e.window.data2);
                    setUpdateCoordinatesAvatar(&digimon);

                    for (i = 0; i <= COUNT_OPERATIONS; i++) {
                        SDL_FRect transform = {
                            .x = i < 4 ? config->widthButton * i
                                       : config->widthButton * (i - 4),
                            .y = i < 4 ? config->overlayArea.y
                                       : config->overlayArea.y +
                                             config->overlayArea.h -
                                             config->heightButton,
                            .w = config->widthButton,
                            .h = config->heightButton};

                        if (i < COUNT_OPERATIONS)
                            buttonsOperations[i].transform = transform;
                        else
                            buttonCallStatus.transform = transform;
                        if (i < COUNT_CONTROL_BUTTON_TYPE)
                            buttonsControl[i].transform =
                                config->overlayButtons[i];
                    }
                }
                break;
            case SDL_MOUSEMOTION:
                updateButtonsHovering(e.motion.x, e.motion.y);
                break;
            case SDL_FINGERUP:
                e.button.button = SDL_BUTTON_LEFT;
                e.button.x = e.tfinger.x * config->widthScreen;
                e.button.y = e.tfinger.y * config->heightScreen;
                // Fallthrough
            case SDL_MOUSEBUTTONUP: {
                int resultClick = updateControlsButtonsClick(
                    e.button.x, e.button.y, &forcedScanCode);
                if (resultClick != NO_OPERATION) {
                    e.button.x = buttonsOperations[resultClick].transform.x;
                    e.button.y = buttonsOperations[resultClick].transform.y;
                }
                if (e.button.button == SDL_BUTTON_LEFT) {
                    if (currentOperation == NO_OPERATION) {
                        currentOperation =
                            updateButtonsClick(e.button.x, e.button.y);
                    }
                }
            } break;
        }
    }

    if (!digimon.initiated)
        currentOperation = BIRTHING;

    currentOperation = handleOperation(currentOperation, selectedOptionMenu);

    updateAvatar(&digimon, getDeltaTime());
    return 1;
}

int updateBackGround(int deltaTime) {
    setFileName();
    if (initAvatarNoTexture(&digimon, saveFile) == 0)
        return 0;

    updateInfoAvatar(&digimon, 1, 0);
    SDL_Log("Updated digimon");

    SDL_Delay(1000 / 60);  // 60 fps
    return 1;
}

void drawGame() {
    SDL_RenderClear(gRenderer);

    SDL_RenderCopyF(gRenderer, background, NULL, &config->overlayArea);

    if (responseOperation != BATTLE_W0RLD) {
        if (currentMenu.countOptions)
            drawMenu(gRenderer, &currentMenu);
        else
            drawAvatar(gRenderer, &digimon);
    } else {
        if (digimon.currentAction == NEGATING) {
            drawAvatar(gRenderer, &digimon);
        } else {
            const SDL_Rect clip = {.x = 0,
                                   .y = config->normalSpriteSize * 4,
                                   .w = config->normalSpriteSize * 2,
                                   .h = config->normalSpriteSize};
            const SDL_FRect transform = {
                .x = config->overlayArea.x,
                .y = config->overlayArea.y + config->heightButton,
                .w = config->overlayArea.w,
                .h = config->heightSprite};
            SDL_RenderCopyF(gRenderer, popup, &clip, &transform);
        }
    }

    int i;
    for (i = 0; i < COUNT_OPERATIONS; i++)
        drawButton(gRenderer, &buttonsOperations[i]);
    drawButton(gRenderer, &buttonCallStatus);

    SDL_RenderCopyF(gRenderer, overlay, NULL, NULL);
    SDL_RenderPresent(gRenderer);

    SDL_Delay(1000 / 60);  // 60 fps
}

void cleanUpGame() {
    int i;

    DIGIHW_saveDigimon(saveFile, &digimon.infoApi);

    freeAvatar(&digimon);
    freeTexture(background);
    freeTexture(overlay);
    for (i = 0; i < COUNT_OPERATIONS; i++) {
        freeButton(&buttonsOperations[i]);

        if (i < COUNT_CONTROL_BUTTON_TYPE)
            freeButton(&buttonsControl[i]);
    }
    freeTexture(popup);
    freeButton(&buttonCallStatus);
    freeMenu(&currentMenu);

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
    SDLNet_Quit();
    TTF_Quit();
    SDL_Quit();
}