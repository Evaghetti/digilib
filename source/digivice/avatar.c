#include "digivice/avatar.h"

#include "digiapi.h"
#include "enums.h"

#include "globals.h"
#include "texture.h"

#include "SDL2/SDL_ttf.h"

#include <ctype.h>
#include <stdlib.h>
#include <time.h>

#define SPEED_FLUSH (-STEP_SPRITE * 50)

static const SDL_Rect initialTransform = {WIDTH_SCREEN / 2 - WIDTH_SPRITE / 2,
                                          HEIGHT_BUTTON, WIDTH_SPRITE,
                                          HEIGHT_SPRITE + STEP_SPRITE};
static const SDL_Rect flushClip = {7 * 8, 0, NORMAL_SIZE_SMALL_SPRITE,
                                   NORMAL_SIZE_SPRITE};

static SDL_Texture* textureAdditional;
static AnimationController additionalAnimations;
static AnimationController animationsForPoop;
static float xOffsetSprites = 0;  // Used for the cleaning animation

int initAvatar(Avatar* ret) {
    int statusInit = DIGI_init(SAVE_FILE) == DIGI_RET_OK;

    if (statusInit) {
        ret->infoApi = DIGI_playingDigimon();
        strncpy(ret->name, ret->infoApi.pstCurrentDigimon->szName,
                sizeof(ret->name));

        char spriteSheetFile[270] = {0};
        int i;
        snprintf(spriteSheetFile, sizeof(spriteSheetFile), "resource/%s.gif",
                 ret->name);
        for (i = 0; i < strlen(spriteSheetFile); i++) {
            spriteSheetFile[i] = tolower(spriteSheetFile[i]);
        }

        ret->spriteSheet = loadTexture(spriteSheetFile);
        if (ret->spriteSheet == NULL)
            return 0;

        ret->transform = initialTransform;

        addAnimation(&ret->animationController, "hatching", 2,
                     createRect(0, 0, 16, 16), 1.f,
                     createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f);
        addAnimation(&ret->animationController, "beingBorn", 3,
                     createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE * 2, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f);
        addAnimation(
            &ret->animationController, "walking", 9, createRect(0, 0, 16, 16),
            1.f,
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
        addAnimation(&ret->animationController, "happy", 4,
                     createRect(0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f);
        addAnimation(&ret->animationController, "negating", 4,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f);
        addAnimation(&ret->animationController, "eating", 7,
                     // Entire
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, 4 * NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     // Bitten 1 time
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, 4 * NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     // Bitten 2 times
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(NORMAL_SIZE_SPRITE, 4 * NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     1.f,
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     1.f);

        // Additional stuff for animations etc.
        textureAdditional = loadTexture("resource/feed.gif");
        // Work around
        // TODO: define that the controlelr should not play the first animation.
        addAnimation(
            &additionalAnimations, "nothing", 1,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 7, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE));

        addAnimation(
            &additionalAnimations, "meat", 4,
            createRect(0, 0, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE),
            2.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 1, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            2.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 2, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            2.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 3, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            1.f);

        addAnimation(
            &additionalAnimations, "vitamin", 4,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 4, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            2.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 5, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            2.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 6, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            2.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 7, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            1.f);
        addAnimation(
            &additionalAnimations, "snore", 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 2, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            1.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 3, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            1.f);

        addAnimation(
            &animationsForPoop, "poop", 2,
            createRect(0, NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE),
            1.f,
            createRect(NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            1.f);
        setCurrentAnimation(&animationsForPoop, "poop");

        if (ret->infoApi.pstCurrentDigimon->uiStage == DIGI_STAGE_EGG) {
            ret->currentAction = HATCHING;
            setCurrentAnimation(&ret->animationController, "hatching");
        } else if (ret->infoApi.uiStats & MASK_SLEEPING) {
            ret->currentAction = SLEEPING;
            setCurrentAction(ret, SLEEPING);
        } else {
            ret->currentAction = WALKING;
            setCurrentAnimation(&ret->animationController, "walking");
        }

        unsigned char events;
        DIGI_updateEventsDeltaTime(0, &events);
        handleEvents(ret, events);

        ret->initiated = 1;
    }

    srand(time(NULL));
    return statusInit;
}

void updateAvatar(Avatar* avatar, const float deltaTime) {
    if (!avatar->initiated)
        return;

    avatar->timePassed += deltaTime;
    if (avatar->timePassed >= 1.f) {
        avatar->secondsPassed++;
        if (avatar->secondsPassed >= 60) {
            unsigned char events;
            DIGI_updateEventsDeltaTime(1, &events);
            avatar->infoApi = DIGI_playingDigimon();

            handleEvents(avatar, events);
            avatar->secondsPassed = 0;
        }

        if (avatar->currentAction == WALKING &&
            avatar->infoApi.pstCurrentDigimon->uiStage > DIGI_STAGE_EGG) {
            if (rand() % 100 < 50) {
                avatar->renderFlags = avatar->renderFlags == SDL_FLIP_HORIZONTAL
                                          ? SDL_FLIP_NONE
                                          : SDL_FLIP_HORIZONTAL;
            }
            const int direction =
                avatar->renderFlags & SDL_FLIP_HORIZONTAL ? -1 : 1;

            avatar->transform.x += STEP_SPRITE * direction;
        } else if (avatar->currentAction == EVOLVING) {
            if (finishedCurrentAnimation(&avatar->animationController)) {
                char spriteSheetFile[270] = {0};
                int i;
                snprintf(spriteSheetFile, sizeof(spriteSheetFile),
                         "resource/%s.gif",
                         avatar->infoApi.pstCurrentDigimon->szName);
                for (i = 0; i < strlen(spriteSheetFile); i++) {
                    spriteSheetFile[i] = tolower(spriteSheetFile[i]);
                }

                freeTexture(avatar->spriteSheet);
                avatar->spriteSheet = loadTexture(spriteSheetFile);
                setCurrentAnimation(&avatar->animationController, "walking");
                avatar->currentAction = WALKING;
            }
        } else if (avatar->currentAction == EATING ||
                   avatar->currentAction == STRENGTHNING) {
            avatar->transform = initialTransform;
            avatar->renderFlags = SDL_FLIP_NONE;
            setCurrentAnimation(&avatar->animationController, "eating");

            if (avatar->currentAction == EATING) {
                DIGI_feedDigimon(1);
                setCurrentAnimation(&additionalAnimations, "meat");

            } else {
                DIGI_stregthenDigimon(1, 2);
                setCurrentAnimation(&additionalAnimations, "vitamin");
            }

            if (finishedCurrentAnimation(&avatar->animationController)) {
                setCurrentAnimation(&avatar->animationController, "walking");
                setCurrentAnimation(&additionalAnimations, "nothing");
                avatar->currentAction = WALKING;
            }
        } else if (avatar->currentAction == HEALING) {
            DIGI_healDigimon(MASK_SICK);
            DIGI_healDigimon(MASK_INJURIED);

            avatar->currentAction = HAPPY;
        }

        if (avatar->currentAction == HAPPY ||
            avatar->currentAction == NEGATING) {
            avatar->transform = initialTransform;
            if (avatar->currentAction == HAPPY)
                setCurrentAnimation(&avatar->animationController, "happy");
            else
                setCurrentAnimation(&avatar->animationController, "negating");

            avatar->renderFlags = avatar->renderFlags == SDL_FLIP_HORIZONTAL
                                      ? SDL_FLIP_NONE
                                      : SDL_FLIP_HORIZONTAL;

            if (finishedCurrentAnimation(&avatar->animationController)) {
                setCurrentAnimation(&avatar->animationController, "walking");
                avatar->currentAction = WALKING;
            }
        }

        if (avatar->currentAction == SLEEPING) {
            setCurrentAnimation(&additionalAnimations, "snore");
        }

        avatar->timePassed = 0.f;
    }

    if (avatar->currentAction == CLEANING) {
        if (xOffsetSprites >= WIDTH_SCREEN + WIDTH_SMALL_SPRITE ||
            avatar->infoApi.uiPoopCount == 0) {
            DIGI_cleanPoop();

            xOffsetSprites = 0;

            setCurrentAction(avatar, WALKING);
            avatar->infoApi = DIGI_playingDigimon();
            avatar->transform = initialTransform;
        } else
            xOffsetSprites += SPEED_FLUSH * deltaTime;
    }

    updateAnimation(&avatar->animationController, deltaTime);
    updateAnimation(&additionalAnimations, deltaTime);
    if (avatar->infoApi.uiPoopCount)
        updateAnimation(&animationsForPoop, deltaTime);
}

void handleEvents(Avatar* avatar, const unsigned char events) {
    if (events & DIGI_EVENT_MASK_EVOLVE) {
        avatar->currentAction = EVOLVING;

        SDL_Log("Evolving to %s", avatar->infoApi.pstCurrentDigimon->szName);
        if (avatar->infoApi.pstCurrentDigimon->uiStage == DIGI_STAGE_BABY_1) {
            setCurrentAnimation(&avatar->animationController, "beingBorn");
            SDL_Log("It's a birth");
        }
    }

    if (events & DIGI_EVENT_MASK_WOKE_UP) {
        SDL_Log("Wake up time has arrived");
        setCurrentAction(avatar, WALKING);
    }

    if (events & DIGI_EVENT_MASK_POOP) {
        SDL_Log("Digimon has pooped! Current amount %d",
                avatar->infoApi.uiPoopCount);
    }

    avatar->calling = (events & DIGI_EVENT_MASK_CALL) != 0;
}

void drawAvatar(SDL_Renderer* render, const Avatar* avatar) {
    if (!avatar->initiated)
        return;

    if (avatar->currentAction == SLEEPING) {
        const SDL_Rect* currentSpriteRect =
            getAnimationFrameClip(&additionalAnimations);

        SDL_Rect transform = {.x = avatar->transform.x + WIDTH_SPRITE,
                              .y = HEIGHT_SMALL_SPRITE,
                              .w = WIDTH_SMALL_SPRITE,
                              .h = HEIGHT_SMALL_SPRITE};
        SDL_RenderCopy(render, textureAdditional, currentSpriteRect,
                       &transform);
        return;
    }

    const SDL_Rect* currentSpriteRect =
        getAnimationFrameClip(&avatar->animationController);

    SDL_Rect alteredAvatarTransform = avatar->transform;
    alteredAvatarTransform.x -= (int)xOffsetSprites;

    SDL_RenderCopyEx(render, avatar->spriteSheet, currentSpriteRect,
                     &alteredAvatarTransform, 0.f, NULL, avatar->renderFlags);

    if (!finishedCurrentAnimation(&additionalAnimations)) {
        currentSpriteRect = getAnimationFrameClip(&additionalAnimations);
        SDL_Rect transform = {.x = avatar->transform.x - WIDTH_SMALL_SPRITE,
                              .y = avatar->transform.y + HEIGHT_SMALL_SPRITE,
                              .w = WIDTH_SMALL_SPRITE,
                              .h = HEIGHT_SMALL_SPRITE};

        SDL_RenderCopy(render, textureAdditional, currentSpriteRect,
                       &transform);
    }

    int i;
    for (i = 0; i < avatar->infoApi.uiPoopCount; i++) {
        SDL_Rect transformPoop = {
            .x = WIDTH_SCREEN - WIDTH_SMALL_SPRITE -
                 WIDTH_SMALL_SPRITE * (i % 2) - (int)xOffsetSprites,
            .y = HEIGHT_BUTTON + ((i < 2) ? HEIGHT_SMALL_SPRITE : 0),
            .w = WIDTH_SMALL_SPRITE,
            .h = HEIGHT_SMALL_SPRITE};
        currentSpriteRect = getAnimationFrameClip(&animationsForPoop);
        SDL_RenderCopy(render, textureAdditional, currentSpriteRect,
                       &transformPoop);
    }

    if (avatar->currentAction == CLEANING) {
        SDL_Rect transform = {WIDTH_SCREEN - xOffsetSprites, HEIGHT_BUTTON,
                              WIDTH_SMALL_SPRITE, HEIGHT_SPRITE};
        SDL_RenderCopy(render, textureAdditional, &flushClip, &transform);
    }
}

void setCurrentAction(Avatar* avatar, Action newAction) {
    avatar->currentAction = newAction;
    avatar->timePassed = 1.f;
    setCurrentAnimation(&additionalAnimations, "nothing");

    DIGI_putSleep(newAction == SLEEPING);

    unsigned char uiEvents;
    DIGI_updateEventsDeltaTime(0, &uiEvents);
    handleEvents(avatar, uiEvents);
    avatar->infoApi = DIGI_playingDigimon();
}

static SDL_Texture* createInfoSurface(Avatar* avatar, SDL_Renderer* renderer) {
    static const SDL_Color transparent = {0, 0, 0, 255};
    SDL_Texture* result = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
        WIDTH_SCREEN, HEIGHT_SCREEN - HEIGHT_BUTTON * 2);
    SDL_Texture* textureHud = loadTexture("resource/hud.png");
    SDL_Rect transform = {0, 0, WIDTH_SPRITE, HEIGHT_SPRITE};

    SDL_SetRenderTarget(renderer, result);
    SDL_RenderClear(renderer);
    SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);

    SDL_RenderCopy(
        renderer, avatar->spriteSheet,
        &avatar->animationController.animations[0].firstFrame->frameClip,
        &transform);

    transform.x += WIDTH_SPRITE;
    transform.w = ((WIDTH_SCREEN - transform.w) / 4) / 2;
    transform.h -= HEIGHT_SMALL_SPRITE;
    SDL_Rect currentHud = {0, 0, 8, 8};
    SDL_RenderCopy(renderer, textureHud, &currentHud, &transform);

    transform.x += transform.w;
    transform.w *= 4;
    SDL_Texture* textureNumber =
        createTextTexture(transparent, "%d yr.", avatar->infoApi.uiAge);
    SDL_RenderCopy(renderer, textureNumber, NULL, &transform);
    SDL_DestroyTexture(textureNumber);

    transform.x += transform.w;
    transform.w /= 4;
    currentHud.x += 8;
    SDL_RenderCopy(renderer, textureHud, &currentHud, &transform);

    transform.x += transform.w;
    transform.w *= 2;
    textureNumber =
        createTextTexture(transparent, "%d G", avatar->infoApi.uiWeight);
    SDL_RenderCopy(renderer, textureNumber, NULL, &transform);
    SDL_DestroyTexture(textureNumber);

    transform.x = WIDTH_SPRITE;
    transform.w = WIDTH_SCREEN - WIDTH_SPRITE;
    transform.y += HEIGHT_SMALL_SPRITE - STEP_SPRITE;
    SDL_Texture* textureName = createTextTexture(
        transparent, avatar->infoApi.pstCurrentDigimon->szName);
    SDL_RenderCopy(renderer, textureName, NULL, &transform);
    SDL_DestroyTexture(textureName);

    SDL_SetRenderTarget(renderer, NULL);

    addRawTexture(result);
    return result;
}

static SDL_Texture* heartInfoSurface(const char* text, const int count,
                                     SDL_Renderer* renderer) {
    static const SDL_Color color = {0, 0, 0, 255};
    SDL_Texture* result = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
        WIDTH_SCREEN, HEIGHT_SCREEN - HEIGHT_BUTTON * 2);

    SDL_SetRenderTarget(renderer, result);
    SDL_RenderClear(renderer);
    SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);

    SDL_Texture* textureText = createTextTexture(color, "%s", text);
    SDL_Rect transform = {0, 0, WIDTH_SCREEN, HEIGHT_SMALL_SPRITE};

    SDL_RenderCopy(renderer, textureText, NULL, &transform);
    SDL_DestroyTexture(textureText);

    SDL_Texture* textureHud = loadTexture("resource/hud.png");
    SDL_Rect spriteClip = {8 * 2, 0, 8, 8};
    transform.x -= STEP_SPRITE;
    transform.y += HEIGHT_SMALL_SPRITE;
    transform.w = WIDTH_SCREEN / 4;

    int i;
    for (i = 0; i < count; i++) {
        SDL_RenderCopy(renderer, textureHud, &spriteClip, &transform);
        transform.x += transform.w;
    }

    spriteClip.x += 8;
    for (; i < 4; i++) {
        SDL_RenderCopy(renderer, textureHud, &spriteClip, &transform);
        transform.x += transform.w;
    }

    SDL_SetRenderTarget(renderer, NULL);

    addRawTexture(result);
    return result;
}

Menu createTexturesInfoMenu(Avatar* avatar, SDL_Renderer* renderer) {
    static SDL_Color color = {0, 0, 0, 255};
    const int currentHunger =
        GET_HUNGER_VALUE(avatar->infoApi.uiHungerStrength);
    const int currentStrength =
        GET_STRENGTH_VALUE(avatar->infoApi.uiHungerStrength);
    const int currentWinPercentage =
        avatar->infoApi.uiBattleCount
            ? avatar->infoApi.uiWinCount / avatar->infoApi.uiBattleCount
            : 0;

    SDL_Texture* textureWins =
        createTextTexture(color, "WINS %d %%", currentWinPercentage);
    addRawTexture(textureWins);

    SDL_Texture* pages[] = {
        createInfoSurface(avatar, renderer),
        heartInfoSurface("HUNGER", currentHunger, renderer),
        heartInfoSurface("STRENGTH", currentStrength, renderer), textureWins};
    Menu retMenu = initMenuImageRaw(4, pages);
    return retMenu;
}

void freeAvatar(Avatar* avatar) {
    freeAnimationController(&additionalAnimations);
    freeAnimationController(&animationsForPoop);
    freeAnimationController(&avatar->animationController);
    if (avatar->spriteSheet)
        freeTexture(avatar->spriteSheet);
    freeTexture(textureAdditional);
}
