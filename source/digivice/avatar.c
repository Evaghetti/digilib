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
#define GAME_TICK   .5f

typedef enum { DOWN, UP } GuessShadowBox;

// DM20 patterns (https://humulos.com/digimon/dm20/manual/)
static GuessShadowBox patternsShadowBox[][5] = {
    {UP, DOWN, UP, DOWN, UP},   {UP, DOWN, UP, DOWN, DOWN},
    {DOWN, DOWN, UP, UP, DOWN}, {DOWN, UP, UP, DOWN, DOWN},
    {UP, DOWN, DOWN, UP, UP},   {DOWN, UP, DOWN, UP, DOWN}};

static const int COUNT_PATTERNS_SHADOWBOX =
    sizeof(patternsShadowBox) / sizeof(patternsShadowBox[0]);

static const SDL_Rect initialTransform = {WIDTH_SCREEN / 2 - WIDTH_SPRITE / 2,
                                          HEIGHT_BUTTON, WIDTH_SPRITE,
                                          HEIGHT_SPRITE + STEP_SPRITE};
static const SDL_Rect flushClip = {7 * 8, 8, NORMAL_SIZE_SMALL_SPRITE,
                                   NORMAL_SIZE_SMALL_SPRITE};

static SDL_Texture *textureAdditional, *textureEnemy, *texturePopup;
static AnimationController additionalAnimations;
static AnimationController animationsForPoop;
static float xOffsetSprites = 0;     // Used for the cleaning animation
static float xProjectileOffset = 0;  // Used for position of projectile
static int offsetTraining = 0, correctTrainingGuess = 0;
static int skipFirstFrameScroll = 0, scrolledTrainingStance = 0;
static int selectOptionTraining = 0;

static SDL_RendererFlags projectileRenderFlags = SDL_FLIP_NONE;
static float xProjectileSpeed = -SPEED_FLUSH;
static int roundBattle = 0, battleResult = 0;

static void updateInfoAvatar(Avatar* avatar, int deltaTime) {
    unsigned char events;

    DIGI_updateEventsDeltaTime(deltaTime, &events);
    handleEvents(avatar, events);
    avatar->infoApi = DIGI_playingDigimon();
}

static void advanceTraining(Avatar* avatar, int hasBeenSuccessful) {
    avatar->timePassed = GAME_TICK;
    avatar->transform = initialTransform;

    setCurrentAnimation(&avatar->animationController, "mad");
    avatar->currentAction = MAD_TRAINING;
    if (hasBeenSuccessful) {
        setCurrentAnimation(&avatar->animationController, "happy");
        avatar->currentAction = HAPPY_TRAINING;
        correctTrainingGuess++;
    }

    offsetTraining++;
    if (offsetTraining >= 5) {
        avatar->currentTraining =
            (avatar->currentTraining + 1) % COUNT_PATTERNS_SHADOWBOX;
        offsetTraining = 0;

        SDL_Log("Amount of successess %d", correctTrainingGuess);
        if (correctTrainingGuess >= 3) {
            SDL_Log("Enough successes to strengthen!");
            DIGI_trainDigimon(1);
        }

        if (avatar->currentAction & HAPPY)
            avatar->currentAction = HAPPY_SCORE;
        else
            avatar->currentAction = MAD_SCORE;
    }
}

static int isOnScreenCenter(const Avatar* avatar) {
    return memcmp(&avatar->transform, &initialTransform,
                  sizeof(avatar->transform)) == 0;
}

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
                     createRect(0, 0, 16, 16), GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "beingBorn", 3,
                     createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, 0, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(
            &ret->animationController, "walking", 9, createRect(0, 0, 16, 16),
            GAME_TICK,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            GAME_TICK, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
            GAME_TICK,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            GAME_TICK, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
            GAME_TICK,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            GAME_TICK, createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
            GAME_TICK,
            createRect(NORMAL_SIZE_SPRITE, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            GAME_TICK,
            createRect(NORMAL_SIZE_SPRITE * 2, 0, NORMAL_SIZE_SPRITE,
                       NORMAL_SIZE_SPRITE),
            GAME_TICK);
        addAnimation(&ret->animationController, "happy", 4,
                     createRect(0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "negating", 4,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "eating", 7,
                     // Entire
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, 4 * NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     // Bitten 1 time
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, 4 * NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     // Bitten 2 times
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE, 4 * NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(0, 4 * NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "preparing", 1,
                     createRect(NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 3,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "shooting", 1,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE * 3,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "mad", 4,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE * 2,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(0, NORMAL_SIZE_SPRITE * 3, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE * 2,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(0, NORMAL_SIZE_SPRITE * 3, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "standing", 1,
                     createRect(0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK);
        addAnimation(&ret->animationController, "sick", 4,
                     createRect(0, NORMAL_SIZE_SPRITE * 5, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE * 4,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(0, NORMAL_SIZE_SPRITE * 5, NORMAL_SIZE_SPRITE,
                                NORMAL_SIZE_SPRITE),
                     GAME_TICK,
                     createRect(NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE * 4,
                                NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE),
                     GAME_TICK);

        // Additional stuff for animations etc.
        textureAdditional = loadTexture("resource/feed.gif");
        texturePopup = loadTexture("resource/popups.gif");
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
            GAME_TICK * 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 1, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK * 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 2, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK * 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 3, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK);

        addAnimation(
            &additionalAnimations, "vitamin", 4,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 4, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK * 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 5, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK * 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 6, 0,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK * 2, createRect(0, 0, 0, 0), GAME_TICK);
        addAnimation(
            &additionalAnimations, "snore", 2,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 2, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK,
            createRect(NORMAL_SIZE_SMALL_SPRITE * 3, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK);
        addAnimation(
            &additionalAnimations, "damage", 8,
            createRect(0, 0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE), 0.15f,
            createRect(0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                       NORMAL_SIZE_SPRITE),
            0.15f, createRect(0, 0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE),
            0.15f,
            createRect(0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                       NORMAL_SIZE_SPRITE),
            0.15f, createRect(0, 0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE),
            0.15f,
            createRect(0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                       NORMAL_SIZE_SPRITE),
            0.15f, createRect(0, 0, NORMAL_SIZE_SPRITE * 2, NORMAL_SIZE_SPRITE),
            0.15f,
            createRect(0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE * 2,
                       NORMAL_SIZE_SPRITE),
            0.15f);

        addAnimation(
            &animationsForPoop, "poop", 2,
            createRect(0, NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK,
            createRect(NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE,
                       NORMAL_SIZE_SMALL_SPRITE, NORMAL_SIZE_SMALL_SPRITE),
            GAME_TICK);
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

        updateInfoAvatar(ret, 0);
        ret->initiated = 1;
    }

    srand(time(NULL));
    return statusInit;
}

void updateAvatar(Avatar* avatar, const float deltaTime) {
    if (!avatar->initiated)
        return;

    avatar->timePassed += deltaTime;
    if (avatar->timePassed >= GAME_TICK) {
        avatar->secondsPassed++;
        if (avatar->secondsPassed >= 60) {
            updateInfoAvatar(avatar, 1);

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
                setCurrentAnimation(&additionalAnimations, "meat");
            } else {
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

        if ((avatar->currentAction & (HAPPY | NEGATING | MAD))) {
            avatar->transform = initialTransform;
            if (avatar->currentAction & HAPPY) {
                setCurrentAnimation(&avatar->animationController, "happy");
                avatar->renderFlags = SDL_FLIP_NONE;
            } else if (avatar->currentAction & MAD) {
                if (avatar->currentAction == SAD_BATTLE) {
                    if (isCurrentAnimationAndFinished(
                            &avatar->animationController, "preparing"))
                        setCurrentAnimation(&avatar->animationController,
                                            "sick");
                } else
                    setCurrentAnimation(&avatar->animationController, "mad");
            } else {
                setCurrentAnimation(&avatar->animationController, "negating");
                avatar->renderFlags = avatar->renderFlags == SDL_FLIP_HORIZONTAL
                                          ? SDL_FLIP_NONE
                                          : SDL_FLIP_HORIZONTAL;
            }

            if (finishedCurrentAnimation(&avatar->animationController)) {
                switch (avatar->currentAction) {
                    case HAPPY_SCORE:
                    case MAD_SCORE:
                        avatar->currentAction = SHOWING_SCORE;
                        avatar->timePassed = GAME_TICK;
                        resetCurrentAnimation(&avatar->animationController);
                        selectOptionTraining = 0;
                        break;
                    case HAPPY_TRAINING:
                    case MAD_TRAINING:
                        setCurrentAnimation(&avatar->animationController,
                                            "preparing");
                        avatar->currentAction = TRAINING;
                        scrolledTrainingStance = 0;
                        selectOptionTraining = 0;
                        break;
                    case SAD_BATTLE:
                        setCurrentAction(avatar, CLEANING_DEFEAT);
                        break;
                    case HAPPY_BATTLE:
                        avatar->timePassed = GAME_TICK;
                        // Fallthrough
                    default:
                        setCurrentAnimation(&avatar->animationController,
                                            "walking");
                        avatar->currentAction = WALKING;
                        break;
                }
            }
        }

        if (avatar->currentAction == SLEEPING) {
            setCurrentAnimation(&additionalAnimations, "snore");
        }

        if (avatar->currentAction & SHOWING_SCORE) {
            if (finishedCurrentAnimation(&avatar->animationController)) {
                if (correctTrainingGuess < 3) {
                    avatar->currentAction = MAD_TRAINING;
                    setCurrentAnimation(&avatar->animationController, "mad");
                } else {
                    avatar->currentAction = HAPPY_TRAINING;
                    setCurrentAnimation(&avatar->animationController, "happy");
                }
                avatar->timePassed = GAME_TICK;
                correctTrainingGuess = 0;
            }
        }
        if (avatar->currentAction & TRAINING) {
            if (avatar->currentAction == TRAINING_UP ||
                avatar->currentAction == TRAINING_DOWN) {
                setCurrentAnimation(&avatar->animationController, "shooting");

                const GuessShadowBox currentAnswer =
                    patternsShadowBox[avatar->currentTraining][offsetTraining];

                if ((avatar->currentAction - TRAINING_DOWN) == currentAnswer) {
                    if (!skipFirstFrameScroll)
                        xProjectileOffset -= WIDTH_SMALL_SPRITE;
                    skipFirstFrameScroll = 0;

                    if (xProjectileOffset <=
                        -WIDTH_SPRITE / 2 + WIDTH_SPRITE * 1.5f) {
                        advanceTraining(avatar, 1);
                    }
                } else if (finishedCurrentAnimation(
                               &avatar->animationController)) {
                    advanceTraining(avatar, 0);
                }
            } else {
                setCurrentAnimation(&avatar->animationController, "preparing");
            }
        }
        if (avatar->currentAction & (BATTLE_STATE | STANDOFF)) {
            if (finishedCurrentAnimation(&avatar->animationController)) {
                if (roundBattle <= 3) {
                    if (isCurrentAnimation(&avatar->animationController,
                                           "preparing")) {
                        xProjectileOffset =
                            avatar->transform.x - WIDTH_SMALL_SPRITE;
                        xProjectileSpeed = -SDL_abs(xProjectileSpeed);
                        setCurrentAnimation(&avatar->animationController,
                                            "shooting");
                    } else if (isCurrentAnimation(&avatar->animationController,
                                                  "shooting")) {
                        setCurrentAnimation(&avatar->animationController,
                                            "standing");
                    } else if (avatar->currentAction == STANDOFF) {
                        setCurrentAnimation(&avatar->animationController,
                                            "preparing");
                        avatar->currentAction = battleResult;

                        avatar->timePassed = GAME_TICK;
                    }
                } else if (avatar->currentAction == BATTLE_WIN) {
                    setCurrentAction(avatar, HAPPY_BATTLE);
                    freeTexture(textureEnemy);
                }
            }
        }
        updateInfoAvatar(avatar, 0);
        avatar->timePassed = 0.f;
    }

    if (avatar->currentAction & CLEANING) {
        xOffsetSprites += SPEED_FLUSH * deltaTime;

        if (avatar->currentAction == CLEANING_DEFEAT) {
            avatar->animationController.timeInCurrentFrame = 0.f;
        }

        if (xOffsetSprites >= WIDTH_SCREEN + WIDTH_SMALL_SPRITE) {
            avatar->transform = initialTransform;
            xOffsetSprites = 0;

            if (avatar->currentAction == CLEANING &&
                avatar->infoApi.uiPoopCount) {
                DIGI_cleanPoop();
            }

            setCurrentAction(avatar, WALKING);
        }
    }

    if (avatar->currentAction == TRAINING) {
        if (xOffsetSprites == 0.f && scrolledTrainingStance == 0) {
            xOffsetSprites = WIDTH_SCREEN * .75f;
        } else if (xOffsetSprites > 0.f) {
            xOffsetSprites -= SPEED_FLUSH * deltaTime;
        } else {
            xOffsetSprites = 0;
            scrolledTrainingStance = 1;
        }
    }

    if (avatar->currentAction & BATTLE_STATE) {
        if (xProjectileOffset <= avatar->transform.x) {
            xProjectileOffset += xProjectileSpeed * deltaTime;

            int loopingRightside =
                xProjectileOffset + WIDTH_SMALL_SPRITE > avatar->transform.x;
            int loopingLeftside = xProjectileOffset < -WIDTH_SMALL_SPRITE;
            if (loopingLeftside || loopingRightside) {
                xProjectileSpeed = -xProjectileSpeed;
                projectileRenderFlags = projectileRenderFlags == SDL_FLIP_NONE
                                            ? SDL_FLIP_HORIZONTAL
                                            : SDL_FLIP_NONE;

                if (loopingRightside) {
                    xProjectileOffset =
                        avatar->transform.x + avatar->transform.w;
                    roundBattle++;

                    if (!isCurrentAnimation(&additionalAnimations, "damage"))
                        setCurrentAnimation(&additionalAnimations, "damage");
                    if (roundBattle > 3 &&
                        avatar->currentAction == BATTLE_WIN) {
                        avatar->renderFlags = SDL_FLIP_HORIZONTAL;

                        setCurrentAnimation(&additionalAnimations, "nothing");
                    }
                }
            }
        }
    }

    updateAnimation(&avatar->animationController, deltaTime);
    updateAnimation(&additionalAnimations, deltaTime);
    if (avatar->infoApi.uiPoopCount)
        updateAnimation(&animationsForPoop, deltaTime);

    if ((avatar->currentAction & BATTLE_STATE) &&
        isCurrentAnimationAndFinished(&additionalAnimations, "damage")) {
        setCurrentAnimation(&additionalAnimations, "nothing");

        setCurrentAnimation(&avatar->animationController, "preparing");
        if (roundBattle > 3 && avatar->currentAction == BATTLE_LOSE) {
            setCurrentAction(avatar, SAD_BATTLE);
            freeTexture(textureEnemy);
        }
    }
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

void drawAvatarNormal(SDL_Renderer* render, const Avatar* avatar) {
    int i, j;

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

    if (avatar->currentAction & CLEANING) {
        SDL_Rect transform = {WIDTH_SCREEN - xOffsetSprites, HEIGHT_BUTTON,
                              WIDTH_SMALL_SPRITE, HEIGHT_SMALL_SPRITE};
        SDL_RenderCopy(render, textureAdditional, &flushClip, &transform);

        transform.y += HEIGHT_SMALL_SPRITE;
        SDL_RenderCopy(render, textureAdditional, &flushClip, &transform);
    }

    if ((avatar->currentAction == SAD_BATTLE ||
         avatar->currentAction == HAPPY_BATTLE ||
         avatar->currentAction == CLEANING_DEFEAT) &&
        isOnScreenCenter(avatar)) {
        const SDL_Rect clip = {.w = NORMAL_SIZE_SMALL_SPRITE,
                               .h = NORMAL_SIZE_SMALL_SPRITE,
                               .x = avatar->currentAction == HAPPY_BATTLE
                                        ? 8 * NORMAL_SIZE_SMALL_SPRITE
                                        : 4 * NORMAL_SIZE_SMALL_SPRITE,
                               .y = avatar->currentAction == HAPPY_BATTLE
                                        ? 0
                                        : NORMAL_SIZE_SMALL_SPRITE};
        SDL_Rect transform = {.x = -xOffsetSprites,
                              .y = 0,
                              .w = WIDTH_SMALL_SPRITE,
                              .h = HEIGHT_SMALL_SPRITE};

        for (i = 0; i < 2; i++) {
            transform.y = HEIGHT_BUTTON;

            for (j = 0; j < 2; j++) {
                SDL_RenderCopy(render, textureAdditional, &clip, &transform);

                transform.y += transform.h;
            }

            transform.x = WIDTH_SCREEN - WIDTH_SMALL_SPRITE - xOffsetSprites;
        }
    } else {
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
    }
}

void drawTrainingScore(SDL_Renderer* render) {
    const SDL_Rect shieldClip = {.x = 7 * NORMAL_SIZE_SMALL_SPRITE,
                                 .y = 0,
                                 .w = NORMAL_SIZE_SMALL_SPRITE,
                                 .h = NORMAL_SIZE_SMALL_SPRITE};

    SDL_Texture* hudTexture = loadTexture("resource/hud.png");
    SDL_Rect botamonSpriteClip = {0, 0, NORMAL_SIZE_SMALL_SPRITE,
                                  NORMAL_SIZE_SMALL_SPRITE};

    SDL_Rect botamonTransform = {WIDTH_SMALL_SPRITE, HEIGHT_BUTTON,
                                 WIDTH_SMALL_SPRITE, HEIGHT_SMALL_SPRITE};
    SDL_Rect shieldTranform = {WIDTH_SMALL_SPRITE * 4, HEIGHT_BUTTON,
                               WIDTH_SMALL_SPRITE, HEIGHT_SMALL_SPRITE};

    SDL_RenderCopy(render, hudTexture, &botamonSpriteClip, &botamonTransform);
    SDL_RenderCopy(render, textureAdditional, &shieldClip, &shieldTranform);

    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Texture* scoreTexture = createTextTexture(
        textColor, "%d\tx\t%d", correctTrainingGuess, 5 - correctTrainingGuess);
    botamonTransform.w = WIDTH_SCREEN - botamonTransform.w * 2;
    botamonTransform.y += botamonTransform.h;
    SDL_RenderCopy(render, scoreTexture, NULL, &botamonTransform);
}

void drawAvatarTraining(SDL_Renderer* render, const Avatar* avatar) {
    if (avatar->currentAction & SHOWING_SCORE) {
        drawTrainingScore(render);
        return;
    }

    SDL_Rect transformAvatar = {
        .x = WIDTH_SCREEN - WIDTH_SPRITE - xOffsetSprites,
        .y = HEIGHT_BUTTON,
        .w = WIDTH_SPRITE,
        .h = HEIGHT_SPRITE};
    SDL_Rect transformShadowAvatar = transformAvatar;
    transformShadowAvatar.x = -xOffsetSprites;

    const SDL_Rect* spriteClip =
        getAnimationFrameClip(&avatar->animationController);
    const SDL_Rect shieldClip = {.x = 7 * NORMAL_SIZE_SMALL_SPRITE,
                                 .y = 0,
                                 .w = NORMAL_SIZE_SMALL_SPRITE,
                                 .h = NORMAL_SIZE_SMALL_SPRITE};
    const SDL_Rect projectileClip = {.x = NORMAL_SIZE_SPRITE,
                                     .y = NORMAL_SIZE_SPRITE * 5,
                                     .w = NORMAL_SIZE_SMALL_SPRITE,
                                     .h = NORMAL_SIZE_SMALL_SPRITE};

    SDL_RenderCopy(render, avatar->spriteSheet, spriteClip, &transformAvatar);
    SDL_RenderCopyEx(render, avatar->spriteSheet, spriteClip,
                     &transformShadowAvatar, 0.f, NULL, SDL_FLIP_HORIZONTAL);

    if (avatar->currentAction == TRAINING_UP ||
        avatar->currentAction == TRAINING_DOWN) {
        transformShadowAvatar.x += transformShadowAvatar.w;
        transformShadowAvatar.w = WIDTH_SMALL_SPRITE;
        transformShadowAvatar.h = HEIGHT_SMALL_SPRITE;
        transformAvatar = transformShadowAvatar;

        if (patternsShadowBox[avatar->currentTraining][offsetTraining] != DOWN)
            transformShadowAvatar.y += transformShadowAvatar.h;

        transformAvatar.x = xProjectileOffset;
        if (avatar->currentAction == TRAINING_DOWN)
            transformAvatar.y += transformAvatar.h;

        SDL_RenderCopy(render, avatar->spriteSheet, &projectileClip,
                       &transformAvatar);
        SDL_RenderCopy(render, textureAdditional, &shieldClip,
                       &transformShadowAvatar);
    }

    int i;
    for (i = 0; i < 4; i++) {
        SDL_Rect transformProjectile = {
            .x = WIDTH_SCREEN + WIDTH_SMALL_SPRITE * (i % 2) -
                 (int)xOffsetSprites,
            .y = HEIGHT_BUTTON + ((i < 2) ? HEIGHT_SMALL_SPRITE : 0),
            .w = WIDTH_SMALL_SPRITE,
            .h = HEIGHT_SMALL_SPRITE};

        SDL_RenderCopy(render, avatar->spriteSheet, &projectileClip,
                       &transformProjectile);
    }
}

void drawAvatarBattle(SDL_Renderer* render, const Avatar* avatar) {
    if (isCurrentAnimation(&additionalAnimations, "damage") &&
        !finishedCurrentAnimation(&additionalAnimations)) {
        SDL_Rect transformDamage = {
            .x = 0, .y = HEIGHT_BUTTON, .w = WIDTH_SCREEN, .h = HEIGHT_SPRITE};
        const SDL_Rect* clipPopup =
            getAnimationFrameClip(&additionalAnimations);

        SDL_RenderCopy(render, texturePopup, clipPopup, &transformDamage);
        return;
    }

    if (avatar->currentAction == STANDOFF) {
        SDL_Rect transformChallenged = avatar->transform;
        transformChallenged.x = 0;

        const SDL_Rect* clip =
            getAnimationFrameClip(&avatar->animationController);
        SDL_RenderCopyEx(render, avatar->spriteSheet, clip, &avatar->transform,
                         0.f, NULL, avatar->renderFlags);
        SDL_RenderCopyEx(render, textureEnemy, clip, &transformChallenged, 0.f,
                         NULL, !avatar->renderFlags);
        return;
    }

    SDL_Rect transformProjectile = {.x = xProjectileOffset,
                                    .y = HEIGHT_BUTTON,
                                    .w = WIDTH_SMALL_SPRITE,
                                    .h = HEIGHT_SMALL_SPRITE};
    const SDL_Rect* playerClip =
        getAnimationFrameClip(&avatar->animationController);
    const SDL_Rect projectileClip = {.x = NORMAL_SIZE_SPRITE,
                                     .y = NORMAL_SIZE_SPRITE * 5,
                                     .w = NORMAL_SIZE_SMALL_SPRITE,
                                     .h = NORMAL_SIZE_SMALL_SPRITE};
    SDL_Texture* textureProjectile =
        xProjectileSpeed < 0 ? avatar->spriteSheet : textureEnemy;

    SDL_RenderCopyEx(render, avatar->spriteSheet, playerClip,
                     &avatar->transform, 0, NULL, avatar->renderFlags);
    SDL_RenderCopyEx(render, textureProjectile, &projectileClip,
                     &transformProjectile, 0, NULL, projectileRenderFlags);
    if (roundBattle == 3) {
        if ((xProjectileSpeed < 0 && avatar->currentAction == BATTLE_WIN) ||
            (xProjectileSpeed > 0 && avatar->currentAction == BATTLE_LOSE))
            transformProjectile.y += transformProjectile.h;
        SDL_RenderCopyEx(render, textureProjectile, &projectileClip,
                         &transformProjectile, 0, NULL, projectileRenderFlags);
    }
}

void drawAvatar(SDL_Renderer* render, const Avatar* avatar) {
    if (!avatar->initiated)
        return;

    if (avatar->currentAction & (TRAINING | SHOWING_SCORE))
        drawAvatarTraining(render, avatar);
    else if (avatar->currentAction & (BATTLE_STATE | STANDOFF))
        drawAvatarBattle(render, avatar);
    else
        drawAvatarNormal(render, avatar);
}

void setCurrentAction(Avatar* avatar, Action newAction) {
    const Action oldAction = avatar->currentAction;
    avatar->currentAction = newAction;
    avatar->timePassed = GAME_TICK;
    setCurrentAnimation(&additionalAnimations, "nothing");
    if (newAction == WALKING) {
        setCurrentAnimation(&avatar->animationController, "walking");
        avatar->transform = initialTransform;
    }

    DIGI_putSleep(newAction == SLEEPING);

    switch (newAction) {
        case EATING:
            DIGI_feedDigimon(1);
            break;
        case STRENGTHNING:
            DIGI_stregthenDigimon(1, 2);
            break;
        case TRAINING:
            offsetTraining = 0;
            correctTrainingGuess = 0;
            break;
        case TRAINING_UP:
        case TRAINING_DOWN:
            if (selectOptionTraining) {
                avatar->currentAction = oldAction;
                break;
            }

            skipFirstFrameScroll = 1;
            xProjectileOffset =
                WIDTH_SCREEN - (WIDTH_SPRITE + WIDTH_SMALL_SPRITE);
            xOffsetSprites = 0;
            selectOptionTraining = 1;
            break;
        default:
            break;
    }

    updateInfoAvatar(avatar, 0);
}

void setBattleAction(Avatar* avatar, StatusUpdate status, SDL_Texture* enemy) {
    battleResult = status & WIN ? BATTLE_WIN : BATTLE_LOSE;
    avatar->currentAction = STANDOFF;
    avatar->transform.x = WIDTH_SCREEN - avatar->transform.w;
    avatar->renderFlags = SDL_FLIP_NONE;
    setCurrentAnimation(&avatar->animationController, "mad");
    xProjectileOffset = avatar->transform.x + WIDTH_SPRITE;
    roundBattle = 0;
    textureEnemy = enemy;
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
    freeTexture(texturePopup);
}
