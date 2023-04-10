#include "info.h"

#include "digivice_hal.h"
#include "render.h"
#include "sprites.h"

#define STEP_SCROLL_TEXT      100
#define INITIAL_POSITION_TEXT 16
#define INITIAL_TIME          -750

#define TITLE_HUNGER   "Hunger"
#define TITLE_STRENGTH "Strength"

typedef enum info_state_e {
    NAME,
    HUNGER,
    STRENGTH,
    COUNT_INFO_STATES
} info_state_e;

static info_state_e eCurrentState;
static const char* pszDigimonName;
static uint16_t uiIndexdigimon;
static int16_t iDeltaTimeDisplay;
static uint8_t uiPositionText, uiSizeName, uiHunger, uiStrength;

void DIGIVICE_initInfoDisplay(const playing_digimon_t* pstPlayerData) {
    eCurrentState = NAME;

    pszDigimonName = pstPlayerData->pstCurrentDigimon->szName;
    uiIndexdigimon = pstPlayerData->uiIndexCurrentDigimon;
    uiHunger = GET_HUNGER_VALUE(pstPlayerData->uiHungerStrength);
    uiStrength = GET_STRENGTH_VALUE(pstPlayerData->uiHungerStrength);

    iDeltaTimeDisplay = INITIAL_TIME;
    uiPositionText = INITIAL_POSITION_TEXT;

    uiSizeName = 0;
    while (*(pszDigimonName + uiSizeName) != '\0')
        uiSizeName++;
}

void DIGIVICE_updateInfoDisplay(uint32_t uiDeltaTime) {
    if (eCurrentState != NAME)
        return;

    iDeltaTimeDisplay += uiDeltaTime;

    if (iDeltaTimeDisplay >= STEP_SCROLL_TEXT) {
        iDeltaTimeDisplay = 0;

        uiPositionText--;
        if (uiPositionText == (255 - (uiSizeName << 2)) + INITIAL_POSITION_TEXT)
            uiPositionText = LCD_SCREEN_WIDTH;
    }
}

void displayHearts(const char* pszTitle, uint8_t uiCount) {
    uint8_t i;
    DIGIVICE_drawText(pszTitle, 0, 0, EFFECT_NONE);

    uiCount <<= 3;
    for (i = 0; i < uiCount; i += 8)
        DIGIVICE_drawTile(FILLED_HEART_TILE, i, 8, EFFECT_NONE);
    for (; i < LCD_SCREEN_WIDTH; i += 8)
        DIGIVICE_drawTile(EMPTY_HEART_TILE, i, 8, EFFECT_NONE);
}

void DIGIVICE_renderInfoDisplay() {
    uint8_t i, j;

    switch (eCurrentState) {
        case NAME:
            for (i = LCD_SCREEN_HEIGHT >> 1; i < LCD_SCREEN_HEIGHT; i++) {
                for (j = LCD_SCREEN_WIDTH >> 1; j < LCD_SCREEN_WIDTH; j++)
                    gpstDigiviceHal->setLCDStatus(j, i, 1);
            }
            DIGIVICE_drawText(pszDigimonName, uiPositionText, 8,
                              EFFECT_REVERSE_COLOR);
            DIGIVICE_drawSprite(
                guiDigimonWalkingAnimationDatabase[uiIndexdigimon][0], 0, 0,
                EFFECT_REVERSE_TILE);
            break;
        case HUNGER:
            displayHearts(TITLE_HUNGER, uiHunger - 3);
            break;
        case STRENGTH:
            displayHearts(TITLE_STRENGTH, uiHunger - 1);
            break;
        default:
            break;
    }
}

void DIGIVICE_advanceInfoDisplay(int8_t iDirection) {
    if (iDirection < 0 && eCurrentState == NAME)
        eCurrentState = COUNT_INFO_STATES - 1;
    else if (eCurrentState + iDirection == COUNT_INFO_STATES) {
        eCurrentState = NAME;
        uiPositionText = INITIAL_POSITION_TEXT;
        iDeltaTimeDisplay = INITIAL_TIME;
    } else
        eCurrentState += iDirection;
}
