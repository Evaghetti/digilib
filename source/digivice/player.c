#include "player.h"

#include "digivice.h"
#include "enums.h"
#include "enums_digivice.h"

#include "digiapi.h"
#include "digihal.h"
#include "render.h"
#include "sprites.h"

#define STEP_TIME_WALKING    500
#define STEP_TIME_HATCHING   75
#define STEP_TIME_EVOLVING   75
#define STEP_TIME_CLEANING   20
#define STEP_TIME_EATING     100
#define STEP_TIME_HAPPY      100
#define STEP_TIME_NEGATING   500
#define STEP_TIME_NEED_SLEEP 1000
#define STEP_TIME_SICK       500
#define STEP_TIME_ANGRY      100
#define ONE_MINUTE           60000

#define FRAME_TO_HATCH_FROM_EGG            50
#define FRAME_TO_CHANGE_FROM_HATCH_TO_WALK (FRAME_TO_HATCH_FROM_EGG + 15)

#define FRAME_TO_START_SMILE_EVOLUTION 15
#define FRAME_TO_START_COUNTING_LINES_EVOLUTION \
    FRAME_TO_START_SMILE_EVOLUTION + 15
#define FRAME_TO_STOP_EVOLUTION 100

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof(x[0]))

#define INSIDE_OF_DIGITAMA(x)                       \
    (x->uiCurrentFrame < FRAME_TO_HATCH_FROM_EGG || \
     x->uiPosition != LCD_CENTER_SPRITE)
#define SHOULD_BE_SMILING(x) \
    (x->uiCurrentFrame >= FRAME_TO_START_SMILE_EVOLUTION)
#define STATE_CAN_SHOW_POOP(x)                                              \
    (x == WALKING || x == EATING || x == EATING_VITAMIN || x == CLEANING || \
     x == HAPPY || x == SICK || x == ANGRY)

#define X_POSITION_SNORE (LCD_CENTER_SPRITE + 16)

static const int8_t uiWalkCycleMove[] = {
    -2, -2, 0, 2, 0,  -2, -2, 2, -2, -2, 2,  0, 0,  0,  2,  2,
    2,  2,  2, 0, -2, 0,  2,  0, 2,  2,  -2, 2, -2, -2, -2, -2};
static const uint8_t uiWalkCycleFlips[] = {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0,
                                           0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1,
                                           1, 0, 1, 1, 1, 1, 1, 0, 0, 0};
static const uint8_t uiWalkCycleIndices[] = {0, 1, 1, 0, 0, 1, 1, 1, 1, 2, 1,
                                             2, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0,
                                             0, 1, 1, 2, 1, 2, 1, 1, 0, 0};

static uint8_t uiEffectFrame = 0, uiCameraOffset = 0;

int DIGIVICE_initPlayer(player_t* pstPlayer) {
    pstPlayer->uiPosition = LCD_CENTER_SPRITE;
    pstPlayer->uiIndexBeforeEvolve = pstPlayer->pstPet->uiIndexCurrentDigimon;
    if (pstPlayer->pstPet->pstCurrentDigimon->uiStage > DIGI_STAGE_EGG) {
        if (DIGI_shouldSleep(pstPlayer->pstPet) == DIGI_RET_OK)
            DIGIVICE_changeStatePlayer(pstPlayer, NEED_SLEEP);
        else
            DIGIVICE_changeStatePlayer(pstPlayer, WALKING);
    } else
        DIGIVICE_changeStatePlayer(pstPlayer, WAITING_HATCH);
    return DIGI_RET_OK;
}

static void updateWaitingHatch(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame++;
    if (pstPlayer->uiCurrentFrame >= 2)
        pstPlayer->uiCurrentFrame = 0;
}

static void updateWaitingHatching(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame++;
    if (INSIDE_OF_DIGITAMA(pstPlayer))
        pstPlayer->uiPosition += pstPlayer->uiFlipped == 0 ? -1 : 1;
    else if (pstPlayer->uiCurrentFrame >= FRAME_TO_CHANGE_FROM_HATCH_TO_WALK)
        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);

    if (pstPlayer->uiPosition <= 6)
        pstPlayer->uiFlipped = 1;
    else if (pstPlayer->uiPosition >= 12)
        pstPlayer->uiFlipped = 0;
}

static void updateWalking(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame++;
    if (pstPlayer->uiCurrentFrame >= SIZE_OF_ARRAY(uiWalkCycleMove))
        pstPlayer->uiCurrentFrame = 0;
    pstPlayer->uiFlipped = uiWalkCycleFlips[pstPlayer->uiCurrentFrame];
    pstPlayer->uiPosition += uiWalkCycleMove[pstPlayer->uiCurrentFrame];
}

static void updateEvolving(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame++;

    if (pstPlayer->uiCurrentFrame >= FRAME_TO_STOP_EVOLUTION) {
        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);
    } else if (pstPlayer->uiCurrentFrame >=
               FRAME_TO_START_COUNTING_LINES_EVOLUTION) {
        if (!pstPlayer->uiFlipped) {
            uiEffectFrame++;
            if (uiEffectFrame == 32) {
                pstPlayer->uiFlipped = 1;
                pstPlayer->uiIndexBeforeEvolve =
                    pstPlayer->pstPet->uiIndexCurrentDigimon;
            }
        } else if (uiEffectFrame > 0) {
            uiEffectFrame--;
        }
    }
}

static void updateEating(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame++;

    if (pstPlayer->uiCurrentFrame == 10) {
        uiEffectFrame++;
        pstPlayer->uiCurrentFrame = 0;
    } else if (pstPlayer->uiCurrentFrame == 5 && uiEffectFrame == 3) {
        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);
    }
}

void updateNeedSleep(player_t* pstPlayer) {
    uiEffectFrame++;

    if (uiEffectFrame == 2) {
        pstPlayer->uiCurrentFrame = (pstPlayer->uiCurrentFrame + 1) & 1;
        uiEffectFrame = 0;
    }
}

static void updateCleaning(player_t* pstPlayer) {
    uiCameraOffset++;

    if (uiCameraOffset >= LCD_SCREEN_WIDTH) {
        if (pstPlayer->pstPet->uiPoopCount) {
            DIGI_cleanPoop(pstPlayer->pstPet);
            DIGIVICE_changeStatePlayer(pstPlayer, HAPPY);
            return;
        }

        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);
    }
}

static void updateNegating(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame++;

    if (pstPlayer->uiCurrentFrame > 6)
        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);
}

void updateSick(player_t* pstPlayer) {
    pstPlayer->uiCurrentFrame = (pstPlayer->uiCurrentFrame + 1) & 1;
}

static uint8_t handleEvents(player_t* pstPlayer, uint8_t uiEvents) {
    if (uiEvents & DIGI_EVENT_MASK_EVOLVE) {
        if (pstPlayer->pstPet->pstCurrentDigimon->uiStage == DIGI_STAGE_BABY_1)
            DIGIVICE_changeStatePlayer(pstPlayer, HATCHING);
        else
            DIGIVICE_changeStatePlayer(pstPlayer, EVOLVING);

        return DIGIVICE_EVENT_HAPPENED;
    }

    if (uiEvents & DIGI_EVENT_MASK_SLEEPY)
        DIGIVICE_changeStatePlayer(pstPlayer, NEED_SLEEP);
    else if (uiEvents & DIGI_EVENT_MASK_WOKE_UP)
        DIGIVICE_changeStatePlayer(pstPlayer, WALKING);

    if (uiEvents & DIGI_EVENT_MASK_SICK)
        DIGIVICE_changeStatePlayer(pstPlayer, SICK);

    return uiEvents ? DIGIVICE_EVENT_HAPPENED : DIGIVICE_RET_OK;
}

int DIGIVICE_updatePlayerLib(player_t* pstPlayer, uint8_t uiMinutes) {
    uint8_t uiEvents, uiRet = DIGI_updateEventsDeltaTime(pstPlayer->pstPet,
                                                         uiMinutes, &uiEvents);
    if (uiRet) {
        LOG("Error on update lib -> %d", uiRet);
        return uiRet;
    }

    uiRet = handleEvents(pstPlayer, uiEvents);
    return uiRet;
}

int DIGIVICE_updatePlayer(player_t* pstPlayer, uint32_t uiDeltaTime) {
    uint8_t uiRet = DIGI_RET_OK;

    pstPlayer->uiDeltaTimeStep += uiDeltaTime;

    if (pstPlayer->uiDeltaTimeStep >= pstPlayer->uiCurrentStep) {
        player_state_e eCurrentState = pstPlayer->eState;

        pstPlayer->uiDeltaTimeStep = 0;

        switch (pstPlayer->eState) {
            case WAITING_HATCH:
                updateWaitingHatch(pstPlayer);
                break;
            case HATCHING:
                updateWaitingHatching(pstPlayer);
                break;
            case WALKING:
                updateWalking(pstPlayer);
                break;
            case EVOLVING:
                updateEvolving(pstPlayer);
                break;
            case EATING:
            case EATING_VITAMIN:
            case HAPPY:
            case ANGRY:
                updateEating(pstPlayer);
                break;
            case NEED_SLEEP:
            case SLEEPING:
                updateNeedSleep(pstPlayer);
                break;
            case CLEANING:
                updateCleaning(pstPlayer);
                break;
            case NEGATING:
                updateNegating(pstPlayer);
                break;
            case SICK:
                updateSick(pstPlayer);
                break;
            default:
                break;
        }

        if (eCurrentState != pstPlayer->eState)
            uiRet |= DIGIVICE_CHANGED_STATE;
    }
    return uiRet;
}

static void drawEvolutionLine(uint8_t uiCountLine) {
    int8_t i, j;
    for (i = 0; i < LCD_SCREEN_HEIGHT && i < uiCountLine; i++) {
        for (j = !(i & 1); j < LCD_SCREEN_WIDTH; j += 2) {
            gpstDigiviceHal->setLCDStatus(j, i, 1);
        }
    }

    if (uiCountLine >= LCD_SCREEN_HEIGHT) {
        uiCountLine = uiCountLine - LCD_SCREEN_HEIGHT;
        for (i = LCD_SCREEN_HEIGHT - 1; i >= LCD_SCREEN_HEIGHT - uiCountLine;
             i--) {
            for (j = (i & 1); j < LCD_SCREEN_WIDTH; j += 2) {
                gpstDigiviceHal->setLCDStatus(j, i, 1);
            }
        }
    }
}

void renderPoops(uint8_t uiCurrentFrame, uint8_t uiPoopCount) {
    int8_t i, j;

    if (uiPoopCount > 8)
        uiPoopCount = 8;

    for (j = LCD_SCREEN_WIDTH - 8;; j -= 8) {
        for (i = 8; i >= 0; i -= 8) {
            if (uiPoopCount == 0)
                return;

            DIGIVICE_drawTile(guiPoopAnimation[uiCurrentFrame],
                              j - uiCameraOffset, i, 0);
            uiPoopCount--;
        }
    }
}

void DIGIVICE_renderPlayer(const player_t* pstPlayer) {
    const uint8_t uiCanShowPoop = pstPlayer->pstPet->uiPoopCount &&
                                  STATE_CAN_SHOW_POOP(pstPlayer->eState);
    const uint8_t uiOffsetPoop = 8 << (pstPlayer->pstPet->uiPoopCount >> 1);
    const uint8_t uiRealPosPlayer = pstPlayer->uiPosition -
                                    (uiCanShowPoop ? uiOffsetPoop : 0) -
                                    uiCameraOffset;
    const uint16_t uiIndexDigimon = pstPlayer->pstPet->uiIndexCurrentDigimon;

    switch (pstPlayer->eState) {
        case CLEANING: {
            uint8_t uiCleanWavePos = 32 - uiCameraOffset;

            DIGIVICE_drawTile(CLEANING_TILE, uiCleanWavePos, 0, EFFECT_NONE);
            DIGIVICE_drawTile(CLEANING_TILE, uiCleanWavePos, 8, EFFECT_NONE);
        }
        case SICK: {
            if (pstPlayer->pstPet->uiStats & MASK_SICK) {
                const uint8_t uiCurrentFrame = pstPlayer->uiCurrentFrame & 1;

                DIGIVICE_drawSprite(
                    guiDigimonAnimationDatabase[uiIndexDigimon][5]
                                               [uiCurrentFrame],
                    uiRealPosPlayer, 0, EFFECT_NONE);
                DIGIVICE_drawTile(guiSkullAnimation[uiCurrentFrame],
                                  uiRealPosPlayer + 16, 0, EFFECT_NONE);
                break;
            }
        }
        case WAITING_HATCH:
        case WALKING: {
            uint8_t uiIndexWalk =
                pstPlayer->uiCurrentFrame == 0xFF
                    ? 0
                    : uiWalkCycleIndices[pstPlayer->uiCurrentFrame];
            DIGIVICE_drawSprite(
                guiDigimonWalkingAnimationDatabase
                    [pstPlayer->pstPet->uiIndexCurrentDigimon][uiIndexWalk],
                uiRealPosPlayer, 0, pstPlayer->uiFlipped);
        } break;
        case HATCHING: {
            const uint16_t* const puiSprite =
                INSIDE_OF_DIGITAMA(pstPlayer)
                    ? guiDigimonWalkingAnimationDatabase
                          [pstPlayer->uiIndexBeforeEvolve][0]
                    : guiDigimonWalkingAnimationDatabase
                          [pstPlayer->uiIndexBeforeEvolve][2];
            DIGIVICE_drawSprite(puiSprite, pstPlayer->uiPosition, 0, 0);
        } break;
        case EVOLVING: {
            uint8_t uiCurrentFrame = SHOULD_BE_SMILING(pstPlayer) ? 1 : 0;
            uint8_t uiIndexOldDigimon = pstPlayer->uiIndexBeforeEvolve;

            DIGIVICE_drawSprite(guiDigimonAnimationDatabase[uiIndexOldDigimon]
                                                           [1][uiCurrentFrame],
                                pstPlayer->uiPosition, 0, 0);
            drawEvolutionLine(uiEffectFrame);
        } break;
        case EATING:
        case EATING_VITAMIN: {
            const uint16_t* const puiSprite =
                pstPlayer->uiCurrentFrame < 5
                    ? guiDigimonAnimationDatabase[uiIndexDigimon][3][0]
                    : guiDigimonAnimationDatabase[uiIndexDigimon][3][1];
            const uint8_t uiPositionItem =
                pstPlayer->uiCurrentFrame < 5 && uiEffectFrame == 0 ? 0 : 8;
            const uint8_t uiIndexFeed = pstPlayer->eState == EATING ? 0 : 1;

            DIGIVICE_drawSprite(puiSprite, uiRealPosPlayer, 0, 0);
            DIGIVICE_drawTile(guiFeedingAnimations[uiIndexFeed][uiEffectFrame],
                              uiRealPosPlayer - 8, uiPositionItem, 0);
        } break;
        case NEED_SLEEP: {
            const uint16_t* const puiSprite =
                guiDigimonAnimationDatabase[uiIndexDigimon][0]
                                           [pstPlayer->uiCurrentFrame];

            DIGIVICE_drawSprite(puiSprite, uiRealPosPlayer, 0, 0);
            DIGIVICE_drawTile(guiSnoreAnimation[uiEffectFrame],
                              X_POSITION_SNORE, 0, 0);
        } break;
        case SLEEPING: {
            uint8_t y, x;

            for (y = 0; y < LCD_SCREEN_HEIGHT; y++) {
                for (x = 0; x < LCD_SCREEN_WIDTH; x++)
                    gpstDigiviceHal->setLCDStatus(x, y, 1);
            }

            if (pstPlayer->pstPet->uiStats & MASK_SLEEPING)
                DIGIVICE_drawTile(guiSnoreAnimation[uiEffectFrame],
                                  pstPlayer->uiPosition + 4, 0,
                                  EFFECT_REVERSE_COLOR);
        } break;
        case HAPPY: {
            const uint16_t* const puiSprite =
                pstPlayer->uiCurrentFrame < 5
                    ? guiDigimonAnimationDatabase[uiIndexDigimon][1][0]
                    : guiDigimonAnimationDatabase[uiIndexDigimon][1][1];

            DIGIVICE_drawSprite(puiSprite, uiRealPosPlayer, 0, 0);
            if (pstPlayer->uiCurrentFrame >= 5) {
                DIGIVICE_drawTile(HAPPY_SUN_TILE, uiRealPosPlayer + 16, 0,
                                  EFFECT_NONE);
            }
        } break;
        case NEGATING: {
            DIGIVICE_drawSprite(
                guiDigimonSingleFrameAnimationDatabase[uiIndexDigimon][0],
                uiRealPosPlayer, 0, pstPlayer->uiCurrentFrame & 1);
        } break;
        case ANGRY: {
            const uint16_t* const puiSprite =
                pstPlayer->uiCurrentFrame < 5
                    ? guiDigimonAnimationDatabase[uiIndexDigimon][2][0]
                    : guiDigimonAnimationDatabase[uiIndexDigimon][2][1];
            const uint8_t* const puiTile = pstPlayer->uiCurrentFrame < 5
                                               ? guiStormAnimation[0]
                                               : guiStormAnimation[1];

            DIGIVICE_drawSprite(puiSprite, uiRealPosPlayer, 0, EFFECT_NONE);
            DIGIVICE_drawTile(puiTile, uiRealPosPlayer + 16, 0, EFFECT_NONE);
        } break;
        default:
            break;
    }

    if (uiCanShowPoop) {
        renderPoops(pstPlayer->uiCurrentFrame & 1,
                    pstPlayer->pstPet->uiPoopCount);
    }
}

uint8_t DIGIVICE_changeStatePlayer(player_t* pstPlayer,
                                   player_state_e eNewState) {
    uint8_t uiRetLib;

    gpstHal->log("Changing from state %d to %d", pstPlayer->eState, eNewState);
    switch (eNewState) {
        case WAITING_HATCH:
            pstPlayer->uiCurrentStep = STEP_TIME_WALKING;
            break;
        case HATCHING:
            pstPlayer->uiCurrentStep = STEP_TIME_HATCHING;
            pstPlayer->uiCurrentFrame = 0x00;
            break;
        case WALKING:
            if (pstPlayer->pstPet->uiStats & MASK_SICK)
                return DIGIVICE_changeStatePlayer(pstPlayer, SICK);

            DIGI_putSleep(pstPlayer->pstPet, 0);
            pstPlayer->uiIndexBeforeEvolve =
                pstPlayer->pstPet->uiIndexCurrentDigimon;
            pstPlayer->uiCurrentStep = STEP_TIME_WALKING;
            pstPlayer->uiCurrentFrame = 0xFF;
            break;
        case EVOLVING:
            pstPlayer->uiCurrentStep = STEP_TIME_EVOLVING;
            uiEffectFrame = 0;
            break;
        case EATING:
        case EATING_VITAMIN:
            if (eNewState == EATING) {
                uiRetLib = DIGI_feedDigimon(pstPlayer->pstPet, 1);

                if (uiRetLib == DIGI_RET_OVERFEED) {
                    pstPlayer->eState = EATING;
                    return DIGIVICE_changeStatePlayer(pstPlayer, NEGATING);
                }
            } else
                DIGI_stregthenDigimon(pstPlayer->pstPet, 1, 2);

            pstPlayer->uiCurrentStep = STEP_TIME_EATING;
            pstPlayer->uiCurrentFrame = 0;
            uiEffectFrame = 0;
            break;
        case NEED_SLEEP:
            pstPlayer->uiCurrentStep = STEP_TIME_NEED_SLEEP;
            pstPlayer->uiCurrentFrame = 0;
            uiEffectFrame = 0;
            break;
        case SLEEPING:
            DIGI_putSleep(pstPlayer->pstPet, 1);
            DIGIVICE_updatePlayerLib(pstPlayer, 0);
            pstPlayer->uiPosition = LCD_CENTER_SPRITE;
            pstPlayer->uiFlipped = 0;
            pstPlayer->eState = eNewState;
            uiEffectFrame = 0;
            return DIGIVICE_RET_OK;
        case CLEANING:
            pstPlayer->eState = eNewState;
            pstPlayer->uiCurrentStep = STEP_TIME_CLEANING;
            return DIGI_RET_OK;
        case SICK:
            pstPlayer->uiCurrentStep = STEP_TIME_SICK;
            pstPlayer->uiCurrentFrame = 0;
            break;
        case NEGATING:
            pstPlayer->uiCurrentStep = STEP_TIME_NEGATING;
            pstPlayer->uiCurrentFrame = 0;
            break;
        case HAPPY:
            if (pstPlayer->pstPet->uiStats & MASK_SICK)
                return DIGIVICE_changeStatePlayer(pstPlayer, SICK);
            pstPlayer->uiCurrentStep = STEP_TIME_HAPPY;
            pstPlayer->uiCurrentFrame = 0;
            uiEffectFrame = 0;
            break;
        case ANGRY:
            pstPlayer->uiCurrentStep = STEP_TIME_ANGRY;
            pstPlayer->uiCurrentFrame = 0;
            uiEffectFrame = 0;
            break;
        case HEALING:
            if (pstPlayer->pstPet->uiStats & (MASK_SICK | MASK_INJURIED)) {
                DIGI_healDigimon(pstPlayer->pstPet, MASK_INJURIED);
                DIGI_healDigimon(pstPlayer->pstPet, MASK_SICK);
                return DIGIVICE_changeStatePlayer(pstPlayer, ANGRY);
            } else {
                return DIGIVICE_changeStatePlayer(pstPlayer, NEGATING);
            }
        case LOSE_BATTLE:
        case WIN_BATTLE:
            break;
        default:
            LOG("Missing %d state", pstPlayer->eState);
            return DIGI_RET_ERROR;
    }

    DIGI_setCalled(pstPlayer->pstPet);
    pstPlayer->uiPosition = LCD_CENTER_SPRITE;
    pstPlayer->uiFlipped = 0;
    pstPlayer->eState = eNewState;
    uiCameraOffset = 0;
    return DIGI_RET_OK;
}
