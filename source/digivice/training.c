#include "training.h"

#include "render.h"
#include "sprites.h"

#define POSITION_PLAYER          24
#define POSITION_BACK_PROJECTILE POSITION_PLAYER + 16
#define POSITION_MIRROR          -8
#define CAMERA_OFFSET_BEGGINING  24

#define STEP_SCROLL 25
#define STEP_WAIT   25

static playing_digimon_t* pstCurrentDigimon;
static uint8_t uiCameraOffset, uiCurrentStep, uiTimeWait;

void DIGIVICE_initTraining(playing_digimon_t* pstPlayingDigimon) {
    pstCurrentDigimon = pstPlayingDigimon;
    uiCameraOffset = CAMERA_OFFSET_BEGGINING;
    uiTimeWait = uiCurrentStep = 0;
}

void DIGIVICE_updateTraining(uint16_t uiDeltaTime) {
    uiCurrentStep += uiDeltaTime;

    if (uiCurrentStep >= STEP_SCROLL) {
        uiCurrentStep = 0;

        if (uiTimeWait < STEP_WAIT)
            uiTimeWait++;
        else if (uiCameraOffset)
            uiCameraOffset--;
    }
}

void DIGIVICE_renderTraining() {
    const uint16_t* const puiSprite =
        guiDigimonAnimationDatabase[pstCurrentDigimon->uiIndexCurrentDigimon -
                                    1][3][0];

    DIGIVICE_drawSprite(puiSprite, POSITION_MIRROR - uiCameraOffset, 0,
                        EFFECT_REVERSE_TILE);
    DIGIVICE_drawSprite(puiSprite, POSITION_PLAYER - uiCameraOffset, 0,
                        EFFECT_NONE);
    if (uiCameraOffset) {
        DIGIVICE_drawTile(guiDigimonProjectileSprites
                              [pstCurrentDigimon->uiIndexCurrentDigimon - 1],
                          POSITION_BACK_PROJECTILE - uiCameraOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(guiDigimonProjectileSprites
                              [pstCurrentDigimon->uiIndexCurrentDigimon - 1],
                          POSITION_BACK_PROJECTILE + 8 - uiCameraOffset, 0,
                          EFFECT_NONE);
        DIGIVICE_drawTile(guiDigimonProjectileSprites
                              [pstCurrentDigimon->uiIndexCurrentDigimon - 1],
                          POSITION_BACK_PROJECTILE - uiCameraOffset, 8,
                          EFFECT_NONE);
        DIGIVICE_drawTile(guiDigimonProjectileSprites
                              [pstCurrentDigimon->uiIndexCurrentDigimon - 1],
                          POSITION_BACK_PROJECTILE + 8 - uiCameraOffset, 8,
                          EFFECT_NONE);
    }
}
