#include "digivice/avatar.h"

#include "digiapi.h"
#include "enums.h"

#include "digivice/globals.h"
#include "digivice/texture.h"

#include <SDL_system.h>
#include <SDL_ttf.h>

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#ifdef _ANDROID_BUILD_
#define NOTIFY_IFTRUE(x, y, e)                                           \
    SDL_Log("(" #x " & (" #y ")) == %d && (lastEvent & (" #y ")) == %d", \
            (x & (y)), (lastEvent & (y)));                               \
    if ((x & (y)) != 0 && (lastEvent & (y)) == 0) {                      \
        SDL_AndroidSendMessage(0x8000, e);                               \
    }
#else
#define NOTIFY_IFTRUE(x, y, e)
#endif

typedef enum { DOWN, UP } GuessShadowBox;

// DM20 patterns (https://humulos.com/digimon/dm20/manual/)
static GuessShadowBox patternsShadowBox[][5] = {
    {UP, DOWN, UP, DOWN, UP},   {UP, DOWN, UP, DOWN, DOWN},
    {DOWN, DOWN, UP, UP, DOWN}, {DOWN, UP, UP, DOWN, DOWN},
    {UP, DOWN, DOWN, UP, UP},   {DOWN, UP, DOWN, UP, DOWN}};

static const int COUNT_PATTERNS_SHADOWBOX =
    sizeof(patternsShadowBox) / sizeof(patternsShadowBox[0]);

static SDL_FRect initialTransform;
static SDL_Rect flushClip;

static SDL_Texture *textureAdditional, *textureEnemy, *texturePopup;
static AnimationController additionalAnimations;
static AnimationController animationsForPoop;
static float xOffsetSprites = 0,
             xOffsetSprites2 = 0;      // Used for the cleaning animation
static float xProjectileOffset = 0.f;  // Used for position of projectile
static int offsetTraining = 0, correctTrainingGuess = 0;
static int skipFirstFrameScroll, decreaseCurtains = 0;
static int selectOptionTraining = 0;

static SDL_RendererFlip projectileRenderFlags = SDL_FLIP_NONE;
static float xProjectileSpeed, flushSpeed;
static int roundBattle = 0, battleResult = 0;

static const Configuration* config;

void updateInfoAvatar(Avatar* avatar, int deltaTime, int hasUi) {
    unsigned char events;

    DIGI_updateEventsDeltaTime(deltaTime, &events);
    avatar->infoApi = DIGI_playingDigimon();
    handleEvents(avatar, events, hasUi);
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

void setUpdateCoordinatesAvatar(Avatar* avatar) {
    config = getConfiguration();

    initialTransform.x = config->widthSprite;
    initialTransform.y = config->overlayArea.y + config->heightButton;
    initialTransform.w = config->widthSprite;
    initialTransform.h = config->heightSprite;

    flushClip.x = 7 * config->normalSmallSpriteSize;
    flushClip.y = config->normalSmallSpriteSize;
    flushClip.w = config->normalSmallSpriteSize;
    flushClip.h = config->normalSmallSpriteSize;

    flushSpeed = (-config->stepSprite * 50);
    xProjectileSpeed = -flushSpeed;

    avatar->transform = initialTransform;
}

int initAvatarNoTexture(Avatar* ret, char* saveGame) {
    int statusInit = DIGI_init(saveGame) == DIGI_RET_OK;

    if (statusInit) {
        ret->infoApi = DIGI_playingDigimon();
        strncpy(ret->name, ret->infoApi.pstCurrentDigimon->szName,
                sizeof(ret->name));

        updateInfoAvatar(ret, 0, 1);
        ret->initiated = 1;
    }

    srand(time(NULL));
    return statusInit;
}

int initAvatar(Avatar* ret, char* saveGame) {
    int statusInit = initAvatarNoTexture(ret, saveGame);

    if (statusInit) {
        setUpdateCoordinatesAvatar(ret);

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

        addAnimation(
            &ret->animationController, "hatching", 2, createRect(0, 0, 16, 16),
            GAME_TICK,
            createRect(config->normalSpriteSize, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "beingBorn", 3,
            createRect(0, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2, 0,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "walking", 9, createRect(0, 0, 16, 16),
            GAME_TICK,
            createRect(config->normalSpriteSize, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(0, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(0, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(0, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 0, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2, 0,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "happy", 4,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, config->normalSpriteSize * 2,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, config->normalSpriteSize * 2,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "negating", 4,
            createRect(config->normalSpriteSize * 2, config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2, config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2, config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2, config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "eating", 7,
            // Entire
            createRect(0, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            // Bitten 1 time
            createRect(0, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            // Bitten 2 times
            createRect(0, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(0, 4 * config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "preparing", 1,
            createRect(config->normalSpriteSize, config->normalSpriteSize * 3,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "shooting", 1,
            createRect(config->normalSpriteSize * 2,
                       config->normalSpriteSize * 3, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "mad", 4,
            createRect(config->normalSpriteSize * 2,
                       config->normalSpriteSize * 2, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(0, config->normalSpriteSize * 3,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2,
                       config->normalSpriteSize * 2, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(0, config->normalSpriteSize * 3,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);
        addAnimation(&ret->animationController, "standing", 1,
                     createRect(0, 0, config->normalSpriteSize,
                                config->normalSpriteSize),
                     GAME_TICK);
        addAnimation(
            &ret->animationController, "sick", 4,
            createRect(0, config->normalSpriteSize * 5,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2,
                       config->normalSpriteSize * 4, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(0, config->normalSpriteSize * 5,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize * 2,
                       config->normalSpriteSize * 4, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK);
        addAnimation(
            &ret->animationController, "happy-short", 2,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, config->normalSpriteSize * 2,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK * 100);
        addAnimation(
            &ret->animationController, "sleeping", 2,
            createRect(0, config->normalSpriteSize, config->normalSpriteSize,
                       config->normalSpriteSize),
            GAME_TICK,
            createRect(config->normalSpriteSize, config->normalSpriteSize,
                       config->normalSpriteSize, config->normalSpriteSize),
            GAME_TICK);

        // Additional stuff for animations etc.
        textureAdditional = loadTexture("resource/feed.gif");
        texturePopup = loadTexture("resource/popups.gif");
        // Work around
        // TODO: define that the controlelr should not play the first animation.
        addAnimation(&additionalAnimations, "nothing", 1,
                     createRect(0, 0, 0, 0), GAME_TICK);

        addAnimation(&additionalAnimations, "meat", 4,
                     createRect(0, 0, config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK * 2,
                     createRect(config->normalSmallSpriteSize * 1, 0,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK * 2,
                     createRect(config->normalSmallSpriteSize * 2, 0,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK * 2,
                     createRect(config->normalSmallSpriteSize * 3, 0,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK);

        addAnimation(&additionalAnimations, "vitamin", 4,
                     createRect(config->normalSmallSpriteSize * 4, 0,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK * 2,
                     createRect(config->normalSmallSpriteSize * 5, 0,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK * 2,
                     createRect(config->normalSmallSpriteSize * 6, 0,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK * 2, createRect(0, 0, 0, 0), GAME_TICK);
        addAnimation(&additionalAnimations, "snore", 2,
                     createRect(config->normalSmallSpriteSize * 2,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK,
                     createRect(config->normalSmallSpriteSize * 3,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK);
        addAnimation(
            &additionalAnimations, "damage", 8,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 3,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 3,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 3,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 2,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f,
            createRect(0, config->normalSpriteSize * 3,
                       config->normalSpriteSize * 2, config->normalSpriteSize),
            0.15f);
        addAnimation(&additionalAnimations, "skull", 2,
                     createRect(4 * config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK,
                     createRect(5 * config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK);

        addAnimation(&animationsForPoop, "poop", 2,
                     createRect(0, config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
                     GAME_TICK,
                     createRect(config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize,
                                config->normalSmallSpriteSize),
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
    }

    return statusInit;
}

void updateAvatar(Avatar* avatar, const float deltaTime) {
    static float timePassedCurtain = 0.f;
    if (!avatar->initiated)
        return;

    avatar->timePassed += deltaTime;
    if (avatar->timePassed >= GAME_TICK) {
        avatar->secondsPassed++;
        if (avatar->secondsPassed >= 60) {
            updateInfoAvatar(avatar, 1, 1);

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

            avatar->transform.x += config->stepSprite * direction;
        } else if (avatar->currentAction == EVOLVING) {
            if (finishedCurrentAnimation(&avatar->animationController) ||
                -xOffsetSprites2 > config->heightSprite) {
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

                if (-xOffsetSprites2 <= config->heightSprite) {
                    setCurrentAnimation(&avatar->animationController,
                                        "walking");
                    avatar->currentAction = WALKING;
                }

                decreaseCurtains = 1;
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
        }

        if ((avatar->currentAction & (HAPPY | NEGATING | MAD))) {
            avatar->transform = initialTransform;

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
                        selectOptionTraining = 0;
                        break;
                    case SAD_BATTLE:
                        if (isCurrentAnimation(&avatar->animationController,
                                               "sick"))
                            setCurrentAction(avatar, CLEANING_DEFEAT);
                        break;
                    case HAPPY_BATTLE:
                        if (isCurrentAnimation(&avatar->animationController,
                                               "happy"))
                            avatar->timePassed = GAME_TICK;
                        else {
                            setCurrentAnimation(&avatar->animationController,
                                                "happy");
                            break;
                        }
                        // Fallthrough
                    default:
                        setCurrentAnimation(&avatar->animationController,
                                            "walking");
                        avatar->currentAction = WALKING;
                        break;
                }
            }

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
            } else if (avatar->currentAction & NEGATING) {
                setCurrentAnimation(&avatar->animationController, "negating");
                avatar->renderFlags = avatar->renderFlags == SDL_FLIP_HORIZONTAL
                                          ? SDL_FLIP_NONE
                                          : SDL_FLIP_HORIZONTAL;
            }
        }

        if (avatar->currentAction == SLEEPING) {
            setCurrentAnimation(&avatar->animationController, "sleeping");
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
                        xProjectileOffset -= config->widthSmallSprite;
                    skipFirstFrameScroll = 0;

                    if (xProjectileOffset <=
                        config->overlayArea.x + config->widthSmallSprite) {
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
                            avatar->transform.x - config->widthSmallSprite;
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
                    if (textureEnemy)
                        freeTexture(textureEnemy);
                }
            }
        }
        updateInfoAvatar(avatar, 0, 1);
        avatar->timePassed = 0.f;
    }

    if (avatar->currentAction & CLEANING) {
        xOffsetSprites += flushSpeed * deltaTime;

        if (avatar->currentAction == CLEANING_DEFEAT) {
            avatar->animationController.timeInCurrentFrame = 0.f;
        }

        if (xOffsetSprites >=
            config->overlayArea.w + config->widthSmallSprite) {
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
        if (xOffsetSprites > 0.f) {
            xOffsetSprites -= flushSpeed * deltaTime;
        } else {
            xOffsetSprites = 0;
        }
    }

    if (avatar->currentAction & BATTLE_STATE) {
        if (xProjectileOffset <= avatar->transform.x) {
            xProjectileOffset += xProjectileSpeed * deltaTime;

            int loopingRightside =
                xProjectileOffset + config->widthSmallSprite >
                avatar->transform.x;
            int loopingLeftside = xProjectileOffset < -config->widthSmallSprite;
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

    if (avatar->currentAction & EVOLVING) {
        if (isCurrentAnimation(&avatar->animationController, "happy-short")) {
            timePassedCurtain += deltaTime;

            if (timePassedCurtain >= GAME_TICK / 8 &&
                !isFirstFrame(&avatar->animationController)) {
                timePassedCurtain = 0.f;

                if (!decreaseCurtains) {
                    if (-xOffsetSprites <= config->heightSprite)
                        xOffsetSprites += config->stepSprite;
                    else if (-xOffsetSprites2 <= config->heightSprite)
                        xOffsetSprites2 += config->stepSprite;
                } else {
                    if (xOffsetSprites2 < 0)
                        xOffsetSprites2 -= config->stepSprite;
                    else if (xOffsetSprites < 0)
                        xOffsetSprites -= config->stepSprite;
                    else {
                        markAnimationAsFinished(&avatar->animationController);
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
            if (textureEnemy)
                freeTexture(textureEnemy);
        }
    }
}

static void sendNotification(const unsigned char events) {
    static unsigned char lastEvent = 0;

    if (lastEvent != events) {
        if (events != 0) {
            NOTIFY_IFTRUE(events, DIGI_EVENT_MASK_EVOLVE, EVOLUTION);
            NOTIFY_IFTRUE(events, DIGI_EVENT_MASK_CALL, CALLING);
            NOTIFY_IFTRUE(events, DIGI_EVENT_MASK_SLEEPY, GOT_SLEEPY);
            NOTIFY_IFTRUE(events, DIGI_EVENT_MASK_WOKE_UP, WOKE);
            NOTIFY_IFTRUE(events, DIGI_EVENT_MASK_DIE, DYING);
            NOTIFY_IFTRUE(events,
                          DIGI_EVENT_MASK_SICK | DIGI_EVENT_MASK_INJURED,
                          TREATMENT);
        }

        lastEvent = events;
    }
}

void handleEvents(Avatar* avatar, const unsigned char events, int hasUi) {
    if (events & DIGI_EVENT_MASK_EVOLVE) {
        avatar->currentAction = EVOLVING;

        SDL_Log("Evolving to %s", avatar->infoApi.pstCurrentDigimon->szName);
        if (avatar->infoApi.pstCurrentDigimon->uiStage == DIGI_STAGE_BABY_1) {
            setCurrentAnimation(&avatar->animationController, "beingBorn");
            SDL_Log("It's a birth");
        } else {
            setCurrentAnimation(&avatar->animationController, "happy-short");
            xOffsetSprites = xOffsetSprites2 = 0;
            decreaseCurtains = 0;
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

    if ((events & DIGI_EVENT_MASK_SLEEPY) && avatar->currentAction == WALKING) {
        setCurrentAnimation(&avatar->animationController, "sick");
        avatar->currentAction = SLEEPY;
        avatar->transform = initialTransform;
        avatar->renderFlags = SDL_FLIP_NONE;
    }

    if ((events & DIGI_EVENT_MASK_SICK) ||
        (avatar->infoApi.uiStats & (MASK_SICK | MASK_INJURIED))) {
        setCurrentAnimation(&avatar->animationController, "sick");
        setCurrentAnimation(&additionalAnimations, "skull");
        avatar->currentAction = SICK;
        avatar->transform = initialTransform;
        avatar->renderFlags = SDL_FLIP_NONE;
    }

    avatar->calling = (events & DIGI_EVENT_MASK_CALL) != 0;

    sendNotification(events);
}

static void drawAvatarEvolution(SDL_Renderer* render, const Avatar* avatar) {
    const int currentLine = xOffsetSprites / config->stepSprite;
    const int currentLine2 = xOffsetSprites2 / config->stepSprite;

    const SDL_Rect clipEvolve = {.x = 0,
                                 .y = config->normalSpriteSize,
                                 .w = config->normalSpriteSize * 2,
                                 .h = currentLine};
    const SDL_Rect clipBlackout = {
        .x = 0, .y = 0, .w = config->normalSpriteSize * 2, .h = currentLine2};

    const SDL_Rect* playerClip =
        getAnimationFrameClip(&avatar->animationController);

    SDL_RenderCopyF(render, avatar->spriteSheet, playerClip,
                    &avatar->transform);

    if (!isFirstFrame(&avatar->animationController)) {
        SDL_FRect transformEvolve = {
            .x = config->overlayArea.x,
            .y = config->overlayArea.y + config->heightButton,
            .w = config->overlayArea.w,
            .h = -config->stepSprite * currentLine};

        SDL_FRect transformBlackout = {
            .x = config->overlayArea.x,
            .y = config->overlayArea.y + config->heightButton,
            .w = config->overlayArea.w,
            .h = -config->stepSprite * currentLine2};

        SDL_RenderCopyF(render, texturePopup, &clipEvolve, &transformEvolve);
        SDL_RenderCopyF(render, texturePopup, &clipBlackout,
                        &transformBlackout);
    }
    SDL_Log("Desenhando evolução");
}

void drawAvatarNormal(SDL_Renderer* render, const Avatar* avatar) {
    int i, j;

    if (avatar->currentAction == EVOLVING) {
        drawAvatarEvolution(render, avatar);
        return;
    }

    const SDL_Rect* currentSpriteRect =
        getAnimationFrameClip(&avatar->animationController);

    SDL_FRect alteredAvatarTransform = avatar->transform;
    alteredAvatarTransform.x -= (int)xOffsetSprites;

    SDL_RenderCopyExF(render, avatar->spriteSheet, currentSpriteRect,
                      &alteredAvatarTransform, 0.f, NULL, avatar->renderFlags);

    if (!finishedCurrentAnimation(&additionalAnimations) ||
        (avatar->currentAction & (SLEEPING | SICK))) {
        currentSpriteRect = getAnimationFrameClip(&additionalAnimations);
        SDL_FRect transform = {
            .x = avatar->currentAction & (SLEEPING | SICK)
                     ? avatar->transform.x + config->widthSprite
                     : avatar->transform.x - config->widthSmallSprite,
            .y = avatar->currentAction & (SLEEPING | SICK)
                     ? avatar->transform.y
                     : avatar->transform.y + config->heightSmallSprite,
            .w = config->widthSmallSprite,
            .h = config->heightSmallSprite};

        SDL_RenderCopyF(render, textureAdditional, currentSpriteRect,
                        &transform);
    }

    if (avatar->currentAction & CLEANING) {
        SDL_FRect transform = {config->overlayArea.w - xOffsetSprites,
                               config->overlayArea.y + config->heightButton,
                               config->widthSmallSprite,
                               config->heightSmallSprite};
        SDL_RenderCopyF(render, textureAdditional, &flushClip, &transform);

        transform.y += config->heightSmallSprite;
        SDL_RenderCopyF(render, textureAdditional, &flushClip, &transform);
    }

    if ((avatar->currentAction == SAD_BATTLE ||
         avatar->currentAction == HAPPY_BATTLE ||
         avatar->currentAction == CLEANING_DEFEAT) &&
        isOnScreenCenter(avatar)) {
        const SDL_Rect clip = {.w = config->normalSmallSpriteSize,
                               .h = config->normalSmallSpriteSize,
                               .x = avatar->currentAction == HAPPY_BATTLE
                                        ? 8 * config->normalSmallSpriteSize
                                        : 4 * config->normalSmallSpriteSize,
                               .y = avatar->currentAction == HAPPY_BATTLE
                                        ? 0
                                        : config->normalSmallSpriteSize};
        SDL_FRect transform = {.x = config->overlayArea.x - xOffsetSprites,
                               .y = config->overlayArea.y,
                               .w = config->widthSmallSprite,
                               .h = config->heightSmallSprite};

        for (i = 0; i < 2; i++) {
            transform.y = config->overlayArea.y + config->heightButton;

            for (j = 0; j < 2; j++) {
                SDL_RenderCopyF(render, textureAdditional, &clip, &transform);

                transform.y += transform.h;
            }

            transform.x = config->overlayArea.w - config->widthSmallSprite -
                          xOffsetSprites;
        }
    } else {
        for (i = 0; i < avatar->infoApi.uiPoopCount; i++) {
            SDL_FRect transformPoop = {
                .x = config->overlayArea.w - config->widthSmallSprite -
                     config->widthSmallSprite * (i % 2) - (int)xOffsetSprites,
                .y = config->overlayArea.y + config->heightButton +
                     ((i < 2) ? config->heightSmallSprite : 0),
                .w = config->widthSmallSprite,
                .h = config->heightSmallSprite};
            currentSpriteRect = getAnimationFrameClip(&animationsForPoop);
            SDL_RenderCopyF(render, textureAdditional, currentSpriteRect,
                            &transformPoop);
        }
    }
}

void drawTrainingScore(SDL_Renderer* render) {
    const SDL_Rect shieldClip = {.x = 7 * config->normalSmallSpriteSize,
                                 .y = 0,
                                 .w = config->normalSmallSpriteSize,
                                 .h = config->normalSmallSpriteSize};

    SDL_Texture* hudTexture = loadTexture("resource/hud.png");
    SDL_Rect botamonSpriteClip = {0, 0, config->normalSmallSpriteSize,
                                  config->normalSmallSpriteSize};

    SDL_FRect botamonTransform = {
        config->overlayArea.x + config->widthSmallSprite,
        config->overlayArea.y + config->heightButton, config->widthSmallSprite,
        config->heightSmallSprite};
    SDL_FRect shieldTranform = {
        config->overlayArea.x + config->widthSmallSprite * 4,
        config->overlayArea.y + config->heightButton, config->widthSmallSprite,
        config->heightSmallSprite};

    SDL_RenderCopyF(render, hudTexture, &botamonSpriteClip, &botamonTransform);
    SDL_RenderCopyF(render, textureAdditional, &shieldClip, &shieldTranform);

    SDL_Color textColor = {0, 0, 0, 255};
    SDL_Texture* scoreTexture = createTextTexture(
        textColor, "%d\tx\t%d", correctTrainingGuess, 5 - correctTrainingGuess);
    botamonTransform.w = config->overlayArea.w - botamonTransform.w * 2;
    botamonTransform.y += botamonTransform.h;
    SDL_RenderCopyF(render, scoreTexture, NULL, &botamonTransform);
    freeTexture(hudTexture);
}

void drawAvatarTraining(SDL_Renderer* render, const Avatar* avatar) {
    if (avatar->currentAction & SHOWING_SCORE) {
        drawTrainingScore(render);
        return;
    }

    SDL_FRect transformAvatar = {
        .x = config->overlayArea.w - config->widthSprite - xOffsetSprites,
        .y = config->overlayArea.y + config->heightButton,
        .w = config->widthSprite,
        .h = config->heightSprite};
    SDL_FRect transformShadowAvatar = transformAvatar;
    transformShadowAvatar.x = -xOffsetSprites;

    const SDL_Rect* spriteClip =
        getAnimationFrameClip(&avatar->animationController);
    const SDL_Rect shieldClip = {.x = 7 * config->normalSmallSpriteSize,
                                 .y = 0,
                                 .w = config->normalSmallSpriteSize,
                                 .h = config->normalSmallSpriteSize};
    const SDL_Rect projectileClip = {.x = config->normalSpriteSize,
                                     .y = config->normalSpriteSize * 5,
                                     .w = config->normalSmallSpriteSize,
                                     .h = config->normalSmallSpriteSize};

    SDL_RenderCopyF(render, avatar->spriteSheet, spriteClip, &transformAvatar);
    SDL_RenderCopyExF(render, avatar->spriteSheet, spriteClip,
                      &transformShadowAvatar, 0.f, NULL, SDL_FLIP_HORIZONTAL);

    if (avatar->currentAction == TRAINING_UP ||
        avatar->currentAction == TRAINING_DOWN) {
        transformShadowAvatar.x += transformShadowAvatar.w;
        transformShadowAvatar.w = config->widthSmallSprite;
        transformShadowAvatar.h = config->heightSmallSprite;
        transformAvatar = transformShadowAvatar;

        if (patternsShadowBox[avatar->currentTraining][offsetTraining] != DOWN)
            transformShadowAvatar.y += transformShadowAvatar.h;

        transformAvatar.x = xProjectileOffset;
        if (avatar->currentAction == TRAINING_DOWN)
            transformAvatar.y += transformAvatar.h;

        SDL_RenderCopyF(render, avatar->spriteSheet, &projectileClip,
                        &transformAvatar);
        SDL_RenderCopyF(render, textureAdditional, &shieldClip,
                        &transformShadowAvatar);
    }

    int i;
    for (i = 0; i < 4; i++) {
        SDL_FRect transformProjectile = {
            .x = config->overlayArea.w + config->widthSmallSprite * (i % 2) -
                 (int)xOffsetSprites,
            .y = config->overlayArea.y + config->heightButton +
                 ((i < 2) ? config->heightSmallSprite : 0),
            .w = config->widthSmallSprite,
            .h = config->heightSmallSprite};

        SDL_RenderCopyF(render, avatar->spriteSheet, &projectileClip,
                        &transformProjectile);
    }
}

void drawAvatarBattle(SDL_Renderer* render, const Avatar* avatar) {
    if (isCurrentAnimation(&additionalAnimations, "damage") &&
        !finishedCurrentAnimation(&additionalAnimations)) {
        SDL_FRect transformDamage = {
            .x = config->overlayArea.x,
            .y = config->overlayArea.y + config->heightButton,
            .w = config->overlayArea.w,
            .h = config->heightSprite};
        const SDL_Rect* clipPopup =
            getAnimationFrameClip(&additionalAnimations);

        SDL_RenderCopyF(render, texturePopup, clipPopup, &transformDamage);
        return;
    }

    if (avatar->currentAction == STANDOFF && textureEnemy != NULL) {
        SDL_FRect transformChallenged = avatar->transform;
        transformChallenged.x = config->overlayArea.x;

        const SDL_Rect* clip =
            getAnimationFrameClip(&avatar->animationController);
        SDL_RenderCopyExF(render, avatar->spriteSheet, clip, &avatar->transform,
                          0.f, NULL, avatar->renderFlags);
        SDL_RenderCopyExF(render, textureEnemy, clip, &transformChallenged, 0.f,
                          NULL, !avatar->renderFlags);
        return;
    }

    SDL_FRect transformProjectile = {
        .x = config->overlayArea.x + xProjectileOffset,
        .y = config->overlayArea.y + config->heightButton,
        .w = config->widthSmallSprite,
        .h = config->heightSmallSprite};
    const SDL_Rect* playerClip =
        getAnimationFrameClip(&avatar->animationController);
    const SDL_Rect projectileClip = {.x = config->normalSpriteSize,
                                     .y = config->normalSpriteSize * 5,
                                     .w = config->normalSmallSpriteSize,
                                     .h = config->normalSmallSpriteSize};
    SDL_Texture* textureProjectile = avatar->spriteSheet;
    if (textureEnemy != NULL && xProjectileSpeed > 0)
        textureProjectile = textureEnemy;

    SDL_RenderCopyExF(render, avatar->spriteSheet, playerClip,
                      &avatar->transform, 0, NULL, avatar->renderFlags);
    SDL_RenderCopyExF(render, textureProjectile, &projectileClip,
                      &transformProjectile, 0, NULL, projectileRenderFlags);
    if (roundBattle == 3) {
        if ((xProjectileSpeed < 0 && avatar->currentAction == BATTLE_WIN) ||
            (xProjectileSpeed > 0 && avatar->currentAction == BATTLE_LOSE))
            transformProjectile.y += transformProjectile.h;
        SDL_RenderCopyExF(render, textureProjectile, &projectileClip,
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
        xOffsetSprites = 0;
    }

    DIGI_putSleep(newAction == SLEEPING);

    switch (newAction) {
        case EATING:
            if (DIGI_feedDigimon(1) == DIGI_RET_OVERFEED) {
                avatar->currentAction = NEGATING;
            }
            break;
        case STRENGTHNING:
            DIGI_stregthenDigimon(1, 2);
            break;
        case TRAINING:
            offsetTraining = 0;
            correctTrainingGuess = 0;
            selectOptionTraining = 0;
            xOffsetSprites = config->overlayArea.w * .75f;
            break;
        case TRAINING_UP:
        case TRAINING_DOWN:
            if (selectOptionTraining) {
                avatar->currentAction = oldAction;
                break;
            }

            skipFirstFrameScroll = 1;
            xProjectileOffset =
                config->overlayArea.w -
                (config->widthSprite + config->widthSmallSprite);
            xOffsetSprites = 0;
            selectOptionTraining = 1;
            break;
        case HEALING:
            DIGI_healDigimon(MASK_SICK);
            DIGI_healDigimon(MASK_INJURIED);

            avatar->currentAction = HAPPY;
            break;
        default:
            break;
    }

    updateInfoAvatar(avatar, 0, 1);
}

void setBattleAction(Avatar* avatar, StatusUpdate status, SDL_Texture* enemy) {
    battleResult = status & WIN ? BATTLE_WIN : BATTLE_LOSE;
    avatar->currentAction = STANDOFF;
    avatar->transform.x = config->overlayArea.w - avatar->transform.w;
    avatar->renderFlags = SDL_FLIP_NONE;
    setCurrentAnimation(&avatar->animationController, "mad");
    xProjectileOffset = avatar->transform.x + config->widthSprite;
    roundBattle = 0;
    textureEnemy = enemy;
}

static SDL_Texture* createInfoSurface(Avatar* avatar, SDL_Renderer* renderer) {
    static const SDL_Color transparent = {0, 0, 0, 255};
    SDL_Texture* result =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_TARGET, config->overlayArea.w,
                          config->overlayArea.h - config->heightButton * 2);
    SDL_Texture* textureHud = loadTexture("resource/hud.png");
    SDL_FRect transform = {0, 0, config->widthSprite, config->heightSprite};

    SDL_SetRenderTarget(renderer, result);
    SDL_RenderClear(renderer);
    SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);

    SDL_RenderCopyF(
        renderer, avatar->spriteSheet,
        &avatar->animationController.animations[0].firstFrame->frameClip,
        &transform);

    transform.x += config->widthSprite;
    transform.w = ((config->overlayArea.w - transform.w) / 4) / 2;
    transform.h -= config->heightSmallSprite;
    SDL_Rect currentHud = {0, 0, 8, 8};
    SDL_RenderCopyF(renderer, textureHud, &currentHud, &transform);

    transform.x += transform.w;
    transform.w *= 4;
    SDL_Texture* textureNumber =
        createTextTexture(transparent, "%d yr.", avatar->infoApi.uiAge);
    SDL_RenderCopyF(renderer, textureNumber, NULL, &transform);
    SDL_DestroyTexture(textureNumber);

    transform.x += transform.w;
    transform.w /= 4;
    currentHud.x += 8;
    SDL_RenderCopyF(renderer, textureHud, &currentHud, &transform);

    transform.x += transform.w;
    transform.w *= 2;
    textureNumber =
        createTextTexture(transparent, "%d G", avatar->infoApi.uiWeight);
    SDL_RenderCopyF(renderer, textureNumber, NULL, &transform);
    SDL_DestroyTexture(textureNumber);

    transform.x = config->widthSprite;
    transform.w = config->overlayArea.w - config->widthSprite;
    transform.y += config->heightSmallSprite - config->stepSprite;
    SDL_Texture* textureName = createTextTexture(
        transparent, avatar->infoApi.pstCurrentDigimon->szName);
    SDL_RenderCopyF(renderer, textureName, NULL, &transform);
    SDL_DestroyTexture(textureName);

    SDL_SetRenderTarget(renderer, NULL);

    addRawTexture(result);
    freeTexture(textureHud);
    return result;
}

static SDL_Texture* heartInfoSurface(const char* text, const int count,
                                     SDL_Renderer* renderer) {
    static const SDL_Color color = {0, 0, 0, 255};
    SDL_Texture* result =
        SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA32,
                          SDL_TEXTUREACCESS_TARGET, config->overlayArea.w,
                          config->overlayArea.h - config->heightButton * 2);

    SDL_SetRenderTarget(renderer, result);
    SDL_RenderClear(renderer);
    SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);

    SDL_Texture* textureText = createTextTexture(color, "%s", text);
    SDL_FRect transform = {0, 0, config->overlayArea.w,
                           config->heightSmallSprite};

    SDL_RenderCopyF(renderer, textureText, NULL, &transform);
    SDL_DestroyTexture(textureText);

    SDL_Texture* textureHud = loadTexture("resource/hud.png");
    SDL_Rect spriteClip = {8 * 2, 0, 8, 8};
    transform.x -= config->stepSprite;
    transform.y += config->heightSmallSprite;
    transform.w = config->overlayArea.w / 4;

    int i;
    for (i = 0; i < count; i++) {
        SDL_RenderCopyF(renderer, textureHud, &spriteClip, &transform);
        transform.x += transform.w;
    }

    spriteClip.x += 8;
    for (; i < 4; i++) {
        SDL_RenderCopyF(renderer, textureHud, &spriteClip, &transform);
        transform.x += transform.w;
    }

    SDL_SetRenderTarget(renderer, NULL);

    addRawTexture(result);
    freeTexture(textureHud);
    return result;
}

Menu createTexturesInfoMenu(Avatar* avatar, SDL_Renderer* renderer) {
    static SDL_Color color = {0, 0, 0, 255};
    const int currentHunger =
        GET_HUNGER_VALUE(avatar->infoApi.uiHungerStrength);
    const int currentStrength =
        GET_STRENGTH_VALUE(avatar->infoApi.uiHungerStrength);
    const int currentWinPercentage =
        avatar->infoApi.uiBattleCount ? ((float)avatar->infoApi.uiWinCount /
                                         (float)avatar->infoApi.uiBattleCount) *
                                            100.f
                                      : 0;

    SDL_Texture* textureWins =
        createTextTexture(color, "WINS %d %%", currentWinPercentage);
    addRawTexture(textureWins);

    SDL_Texture* pages[] = {
        createInfoSurface(avatar, renderer),
        heartInfoSurface("HUNGER", currentHunger, renderer),
        heartInfoSurface("STRENGTH", currentStrength, renderer), textureWins};
    Menu retMenu = initMenuImageRaw(4, pages);
    retMenu.customs = FILL_SCREEN | NO_CURSOR;
    return retMenu;
}

void freeAvatar(Avatar* avatar) {
    freeAnimationController(&additionalAnimations);
    freeAnimationController(&animationsForPoop);
    freeAnimationController(&avatar->animationController);
    if (avatar->spriteSheet)
        freeTexture(avatar->spriteSheet);
    if (textureAdditional)
        freeTexture(textureAdditional);
    if (texturePopup)
        freeTexture(texturePopup);
    if (textureEnemy)
        freeTexture(textureEnemy);
    memset(avatar, 0, sizeof(Avatar));
}
