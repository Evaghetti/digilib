#include "info.h"

#include "digivice_hal.h"
#include "render.h"
#include "sprites.h"

#define STEP_SCROLL_TEXT 100

static const char* pszDigimonName;
static uint16_t uiIndexdigimon;
static int16_t iDeltaTimeDisplay;
static uint8_t uiPositionText, uiSizeName;

void DIGIVICE_initInfoDisplay(const playing_digimon_t* pstPlayerData) {
    pszDigimonName = pstPlayerData->pstCurrentDigimon->szName;
    uiIndexdigimon = pstPlayerData->uiIndexCurrentDigimon;

    iDeltaTimeDisplay = -20;
    uiPositionText = 16;

    uiSizeName = 0;
    while (*(pszDigimonName + uiSizeName) != '\0')
        uiSizeName++;
}

void DIGIVICE_updateInfoDisplay(uint32_t uiDeltaTime) {
    iDeltaTimeDisplay += uiDeltaTime;

    if (iDeltaTimeDisplay >= STEP_SCROLL_TEXT) {
        iDeltaTimeDisplay = 0;

        uiPositionText--;
        if (uiPositionText == (255 - (uiSizeName << 2)) + 16)
            uiPositionText = LCD_SCREEN_WIDTH;
    }
}

void DIGIVICE_renderInfoDisplay() {
    uint8_t i, j;

    for (i = LCD_SCREEN_HEIGHT >> 1; i < LCD_SCREEN_HEIGHT; i++) {
        for (j = LCD_SCREEN_WIDTH >> 1; j < LCD_SCREEN_WIDTH; j++)
            gpstDigiviceHal->setLCDStatus(j, i, 1);
    }
    DIGIVICE_drawText(pszDigimonName, uiPositionText, 8, EFFECT_REVERSE_COLOR);
    DIGIVICE_drawSprite(guiDigimonWalkingAnimationDatabase[uiIndexdigimon][0],
                        0, 0, EFFECT_REVERSE_TILE);
}
