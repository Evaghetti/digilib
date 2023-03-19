#include "player.h"

#include "enums.h"
#include "digivice.h"

#include "digiapi.h"
#include "digihal.h"
#include "render.h"
#include "sprites.h"

#define STEP_TIME_WALKING                   500
#define STEP_TIME_HATCHING                  75
#define STEP_TIME_EVOLVING                  75
#define ONE_MINUTE                          60000

#define FRAME_TO_HATCH_FROM_EGG             50
#define FRAME_TO_CHANGE_FROM_HATCH_TO_WALK (FRAME_TO_HATCH_FROM_EGG + 15)

#define FRAME_TO_START_SMILE_EVOLUTION          15
#define FRAME_TO_START_COUNTING_LINES_EVOLUTION FRAME_TO_START_SMILE_EVOLUTION + 15
#define FRAME_TO_STOP_EVOLUTION                 100

#define SIZE_OF_ARRAY(x) (sizeof(x) / sizeof(x[0]))
#define DEFAULT_ERROR_CHANGING_STATE(x) LOG("Swapping from state %d to %d is not permitted",x->eState, eNewState); return DIGI_RET_ERROR
#define INSIDE_OF_DIGITAMA(x) \
    (x->uiCurrentFrame < FRAME_TO_HATCH_FROM_EGG || x->uiPosition != LCD_CENTER_SPRITE)
#define SHOULD_BE_SMILING(x) \
    (x->uiCurrentFrame >= FRAME_TO_START_SMILE_EVOLUTION)

static const int8_t uiWalkCycleMove[] = {
    -2, -2, 0, 2, 0,  -2, -2, 2, -2, -2, 2,  0, 0,  0,  2,  2,
    2,  2,  2, 0, -2, 0,  2,  0, 2,  2,  -2, 2, -2, -2, -2, -2};
static const uint8_t uiWalkCycleFlips[] = {0, 0, 1, 1, 0, 0, 0, 1, 0, 0, 0,
                                           0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1,
                                           1, 0, 1, 1, 1, 1, 1, 0, 0, 0};
static const uint8_t uiWalkCycleIndices[] = {0, 1, 1, 0, 0, 1, 1, 1, 1, 2, 1,
                                             2, 1, 1, 1, 0, 0, 0, 1, 1, 0, 0,
                                             0, 1, 1, 2, 1, 2, 1, 1, 0, 0};

static uint8_t uiLineEvolutionCount = 0;

int DIGIVICE_initPlayer(player_t* pstPlayer) {
    pstPlayer->uiPosition = LCD_CENTER_SPRITE;
    pstPlayer->uiIndexBeforeEvolve = pstPlayer->pstPet->uiIndexCurrentDigimon;
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
    else  if (pstPlayer->uiCurrentFrame >= FRAME_TO_CHANGE_FROM_HATCH_TO_WALK)
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
    }
    else if (pstPlayer->uiCurrentFrame >= FRAME_TO_START_COUNTING_LINES_EVOLUTION) {
        if (!pstPlayer->uiFlipped) {
            uiLineEvolutionCount++;
            if (uiLineEvolutionCount == 32) {
                pstPlayer->uiFlipped = 1;
                pstPlayer->uiIndexBeforeEvolve =
                    pstPlayer->pstPet->uiIndexCurrentDigimon;
            }
        }
        else if (uiLineEvolutionCount > 0) {
            uiLineEvolutionCount--;
        }
    }
}

static void handleEvents(player_t* pstPlayer, uint8_t uiEvents) {
    if (uiEvents & DIGI_EVENT_MASK_EVOLVE) {
        if (pstPlayer->pstPet->pstCurrentDigimon->uiStage == DIGI_STAGE_BABY_1)
            DIGIVICE_changeStatePlayer(pstPlayer, HATCHING);
        else
            DIGIVICE_changeStatePlayer(pstPlayer, EVOLVING);
    }
}

int DIGIVICE_updatePlayer(player_t* pstPlayer, uint32_t uiDeltaTime) {
    pstPlayer->uiDeltaTimeLib += uiDeltaTime;
    pstPlayer->uiDeltaTimeStep += uiDeltaTime;

    if (pstPlayer->uiDeltaTimeLib >= ONE_MINUTE) {
        pstPlayer->uiDeltaTimeLib = 0;

        uint8_t uiEvents,
            uiRet = DIGI_updateEventsDeltaTime(pstPlayer->pstPet, 1, &uiEvents);
        if (uiRet) {
            LOG("Error on update lib -> %d", uiRet);
            return uiRet;
        }

        handleEvents(pstPlayer, uiEvents);
        pstPlayer->uiDeltaTimeLib = 50000;
    }

    if (pstPlayer->uiDeltaTimeStep >= pstPlayer->uiCurrentStep) {
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
            default:
                break;
        }
    }
    return DIGI_RET_OK;
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
        for (i = LCD_SCREEN_HEIGHT - 1; i >= LCD_SCREEN_HEIGHT - uiCountLine; i--) {
            for (j = (i & 1); j < LCD_SCREEN_WIDTH; j += 2) {
                gpstDigiviceHal->setLCDStatus(j, i, 1);
            }
        }
    }
}

void DIGIVICE_renderPlayer(const player_t* pstPlayer) {
    switch (pstPlayer->eState) {
    case WAITING_HATCH:
    case WALKING:{
        uint8_t uiIndexWalk =
            pstPlayer->uiCurrentFrame == 0xFF
                ? 0
                : uiWalkCycleIndices[pstPlayer->uiCurrentFrame];
        DIGIVICE_drawSprite(
            guiDigimonWalkingAnimationDatabase
                [pstPlayer->pstPet->uiIndexCurrentDigimon][uiIndexWalk],
            pstPlayer->uiPosition, 0, pstPlayer->uiFlipped);
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
        uint8_t uiIndexDigimon = pstPlayer->uiIndexBeforeEvolve - 1;

        DIGIVICE_drawSprite(
            guiDigimonAnimationDatabase[uiIndexDigimon][1][uiCurrentFrame],
            pstPlayer->uiPosition, 0, 0);
        drawEvolutionLine(uiLineEvolutionCount);
    } break;
    default:
        break;
    }
}

static void prepareForWalking(player_t* pstPlayer) {
    pstPlayer->uiIndexBeforeEvolve = pstPlayer->pstPet->uiIndexCurrentDigimon;
    pstPlayer->uiCurrentStep = STEP_TIME_WALKING;
    pstPlayer->uiCurrentFrame = 0xFF;
}

uint8_t DIGIVICE_changeStatePlayer(player_t* pstPlayer, player_state_e eNewState) {    
    switch (pstPlayer->eState) {
    case WAITING_HATCH:
        switch (eNewState) {
            case WAITING_HATCH:
                pstPlayer->uiCurrentStep = STEP_TIME_WALKING;
                break;
            case HATCHING:
                pstPlayer->uiCurrentStep = STEP_TIME_HATCHING;
                pstPlayer->uiCurrentFrame = 0x00;
                break;
            default:
                DEFAULT_ERROR_CHANGING_STATE(pstPlayer);
        }
        break;
    case HATCHING:
        switch (eNewState) {
        case WALKING:
            prepareForWalking(pstPlayer);
            break;
        default:
            DEFAULT_ERROR_CHANGING_STATE(pstPlayer);
        }
        break;
    case WALKING:
        switch (eNewState) {
        case EVOLVING:
            pstPlayer->uiCurrentStep = STEP_TIME_EVOLVING;
            uiLineEvolutionCount = 0;
            break;

        default:
            DEFAULT_ERROR_CHANGING_STATE(pstPlayer);
        }
        break;
    case EVOLVING:
        switch (eNewState) {
        case WALKING:
            prepareForWalking(pstPlayer);
            break;

        default:
            DEFAULT_ERROR_CHANGING_STATE(pstPlayer);
        }
        break;
    default:
        LOG("Missing %d state", pstPlayer->eState);
        return DIGI_RET_ERROR;
    }

    pstPlayer->uiPosition = LCD_CENTER_SPRITE;
    pstPlayer->uiFlipped = 0;
    pstPlayer->eState = eNewState;
    return DIGI_RET_OK;
}
