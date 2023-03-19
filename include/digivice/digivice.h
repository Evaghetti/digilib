#ifndef DIGIVICE_H
#define DIGIVICE_H

#include "digihal.h"
#include "digitype.h"
#include "digivice_hal.h"

typedef enum digivice_icons_e {
    INFO,
    FEED,
    TRAINING,
    BATTLE,
    CLEAN,
    LIGHTS,
    HEAL,
    CALL,
    COUNT_ICONS
} digivice_icons_e;

typedef enum digivice_buttons_e {
    BUTTON_A,
    BUTTON_B,
    BUTTON_C,
    BUTTON_R
} digivice_buttons_e;

uint8_t DIGIVICE_init(const digihal_t* pstHal,
                      const digivice_hal_t* pstDigiviceHal,
                      size_t uiFrequency);

uint8_t DIGIVICE_update();

void DIGIVICE_setButtonState(digivice_buttons_e eButton, uint8_t uiState);

uint8_t DIGIVICE_isButtonPressed(digivice_buttons_e eButton);

#endif  // DIGIVICE_H