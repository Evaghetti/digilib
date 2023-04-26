#include "training.h"

#include "digivice.h"
#include "enums_digivice.h"
#include "render.h"
#include "sprites.h"

#define POSITION_PLAYER          24
#define POSITION_BACK_PROJECTILE POSITION_PLAYER + 16
#define POSITION_MIRROR          -8
#define CAMERA_OFFSET_BEGGINING  24

#define STEP_SCROLL     25
#define STEP_WAIT       25
#define STEP_PROJECTILE 232
#define STEP_SCORE      232

#define POSITION_NOT_SET 0xff

typedef enum training_state_e {
    SCROLLING,
    WAITING,
    SHOTTING,
    SPRITE_ANIMATION,
    SHOW_SCORE,
} training_state_e;

static player_t* pstPlayer;
static training_state_e eCurrentState;
static uint8_t uiCameraOffset, uiCurrentStep, uiTimeWait;
static uint8_t uiFrame, uiXPosProjectile, uiYPosProjectile;
static uint8_t uiCurrentOption, uiCountCorrect, uiCurrentPattern = 0;
static uint8_t uiCurrentLimitStep;

static const uint8_t uiPatterns[10][5] = {
    {0, 0, 0, 1, 1}, {1, 0, 1, 0, 0}, {0, 1, 0, 0, 1}, {1, 0, 0, 1, 0},
    {1, 1, 1, 0, 1}, {0, 0, 1, 1, 0}, {1, 0, 0, 0, 1}, {0, 1, 1, 0, 0},
    {0, 1, 0, 1, 0}, {1, 0, 1, 1, 1}};

static void setStateScrolling() {
    uiCameraOffset = CAMERA_OFFSET_BEGGINING;
    uiXPosProjectile = POSITION_PLAYER - 8;
    uiYPosProjectile = POSITION_NOT_SET;

    uiFrame = uiTimeWait = uiCurrentStep = 0;

    eCurrentState = SCROLLING;
    uiCurrentLimitStep = STEP_SCROLL;
}

void DIGIVICE_initTraining(player_t* pstPlayerRef) {
    pstPlayer = pstPlayerRef;

    uiCurrentOption = 0;
    uiCountCorrect = 0;

    setStateScrolling();
}

static inline void updateCamera() {
    if (uiTimeWait < STEP_WAIT)
        uiTimeWait++;
    else if (uiCameraOffset)
        uiCameraOffset--;
    else {
        eCurrentState = WAITING;
        uiCurrentStep = STEP_WAIT;
    }
}

static inline void updateProjectile() {

    if (uiTimeWait == 2) {
        uiCurrentOption++;
        if (uiCurrentOption >= 5) {
            uiCurrentOption = 0;

            uiCurrentPattern++;
            if (uiCurrentPattern >= 10)
                uiCurrentPattern = 0;
        }

        eCurrentState = SPRITE_ANIMATION;
        uiCurrentLimitStep = 0;
        DIGIVICE_changeStatePlayer(pstPlayer,
                                   uiXPosProjectile >= 16 ? ANGRY : HAPPY);
        return;
    }

    if ((uiYPosProjectile >> 3) !=
            uiPatterns[uiCurrentPattern][uiCurrentOption] &&
        uiXPosProjectile >= 16) {
        uiXPosProjectile -= 8;
        uiCountCorrect++;
    }

    uiTimeWait++;
}

static void changeStateAnimation() {
    // If the current option didn't reset, needs to scroll
    // Using uiTimeWait to check if we are coming back from a score
    // if we are, uiTimeWait will be set to 0.
    if (uiCurrentOption || !uiTimeWait) {
        setStateScrolling();
        return;
    }

    // Otherwise it's time to display the score.
    eCurrentState = SHOW_SCORE;
    uiCurrentLimitStep = STEP_SCORE;
    uiTimeWait = 0;
}

static inline void updateAnimation(uint16_t uiDeltaTime) {
    uint8_t uiRet = DIGIVICE_updatePlayer(pstPlayer, uiDeltaTime);

    if (uiRet & DIGIVICE_CHANGED_STATE) {
        changeStateAnimation();
    }
}

static void updateScore() {
    if (uiTimeWait >= 5) {
        eCurrentState = SPRITE_ANIMATION;
        uiCurrentLimitStep = 0;
        uiTimeWait = 0;
        DIGIVICE_changeStatePlayer(pstPlayer,
                                   uiCountCorrect >= 3 ? HAPPY : ANGRY);

        uiCountCorrect = 0;
        return;
    }
    uiTimeWait++;
}

void DIGIVICE_updateTraining(uint16_t uiDeltaTime) {
    uiCurrentStep += uiDeltaTime;

    if (uiCurrentStep >= uiCurrentLimitStep) {
        uiCurrentStep = 0;

        switch (eCurrentState) {
            case SCROLLING:
                updateCamera();
                break;
            case SHOTTING:
                updateProjectile();
                break;
            case SPRITE_ANIMATION:
                updateAnimation(uiDeltaTime);
                break;
            case SHOW_SCORE:
                updateScore();
                break;
            default:
                break;
        }
    }
}

uint8_t DIGIVICE_handleInputTraining() {
    if (eCurrentState == WAITING) {
        if (DIGIVICE_isButtonPressed(BUTTON_A))
            uiYPosProjectile = 0;
        if (DIGIVICE_isButtonPressed(BUTTON_B))
            uiYPosProjectile = 8;

        if (uiYPosProjectile != POSITION_NOT_SET) {
            eCurrentState = SHOTTING;
            uiCurrentLimitStep = STEP_PROJECTILE;

            uiXPosProjectile = POSITION_PLAYER - 8;
            uiCurrentStep = 0;
            uiFrame = 1;
            uiTimeWait = 0;
        }
    }

    if (eCurrentState == SPRITE_ANIMATION) {
        if (DIGIVICE_isButtonPressed(BUTTON_A) ||
            DIGIVICE_isButtonPressed(BUTTON_B))
            changeStateAnimation();
    }

    if (DIGIVICE_isButtonPressed(BUTTON_C)) {
        if (uiCurrentOption) {
            uiCurrentPattern++;
            uiCurrentOption = 0;
        }
        return DIGIVICE_CHANGED_STATE;
    }

    return DIGIVICE_RET_OK;
}

void DIGIVICE_renderTraining() {
    if (eCurrentState == SPRITE_ANIMATION) {
        DIGIVICE_renderPlayer(pstPlayer);
        return;
    }

    if (eCurrentState == SHOW_SCORE) {
        DIGIVICE_drawTile(AGE_INFO_TILE, 2, 1, EFFECT_NONE);
        DIGIVICE_drawNumber(uiCountCorrect, 4, 7, EFFECT_NONE);
        DIGIVICE_drawText("VS", 12, 7, EFFECT_NONE);
        DIGIVICE_drawNumber(5 - uiCountCorrect, 24, 7, EFFECT_NONE);
        DIGIVICE_drawTile(SHIELD_TILE, 22, 0, EFFECT_NONE);
        return;
    }

    const uint16_t uiIndexDigimon =
        pstPlayer->pstPet->uiIndexCurrentDigimon - 1;
    const uint16_t* const puiSprite =
        guiDigimonAnimationDatabase[uiIndexDigimon][3][uiFrame];
    const uint8_t* puiProjectileTile =
        guiDigimonProjectileSprites[uiIndexDigimon];

    DIGIVICE_drawSprite(puiSprite, POSITION_MIRROR - uiCameraOffset, 0,
                        EFFECT_REVERSE_TILE);
    DIGIVICE_drawSprite(puiSprite, POSITION_PLAYER - uiCameraOffset, 0,
                        EFFECT_NONE);
    if (eCurrentState == SHOTTING) {
        DIGIVICE_drawTile(puiProjectileTile, uiXPosProjectile, uiYPosProjectile,
                          EFFECT_NONE);
        DIGIVICE_drawTile(SHIELD_TILE, POSITION_MIRROR + 16,
                          uiPatterns[uiCurrentPattern][uiCurrentOption] << 3,
                          EFFECT_NONE);
    }

    if (eCurrentState == SCROLLING) {
        DIGIVICE_drawTile(puiProjectileTile,
                          POSITION_BACK_PROJECTILE - uiCameraOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(puiProjectileTile,
                          POSITION_BACK_PROJECTILE + 8 - uiCameraOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(puiProjectileTile,
                          POSITION_BACK_PROJECTILE - uiCameraOffset, 8,
                          EFFECT_NONE);
        DIGIVICE_drawTile(puiProjectileTile,
                          POSITION_BACK_PROJECTILE + 8 - uiCameraOffset, 8,
                          EFFECT_NONE);
    }
}
