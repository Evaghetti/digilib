#include "clock.h"
#include <stdint.h>
#include "render.h"
#include "sprites.h"

#define TICK_AMOUNT 5

static uint8_t uiHour = 0, uiMinute = 0, uiSeconds = 0;
static uint8_t uiTicks[TICK_AMOUNT] = {0}, *puiCurrentTick = uiTicks;
static uint8_t uiLastMinute = 0, fIsConfiguring;

uint16_t DIGIVICE_getTime() {
    return uiHour * 60 + uiMinute;
}

uint8_t DIGIVICE_minutesPassed() {
    if (uiLastMinute == uiMinute)
        return 0;

    uint8_t uiPassedTime = uiMinute - uiLastMinute;
    uiLastMinute = uiMinute;
    return uiPassedTime;
}

void DIGIVICE_toggleSetTime() {
    fIsConfiguring = !fIsConfiguring;

    if (fIsConfiguring) {
        uiSeconds = 0;

        uint8_t i;
        for (i = 0; i < TICK_AMOUNT; i++)
            uiTicks[i] = 0;
        puiCurrentTick = uiTicks;
    }
}

static inline void updateNotConfigurin(uint16_t uiDeltaTime) {
    static uint16_t uiPassedTime = 0;

    uiPassedTime += uiDeltaTime;
    if (uiPassedTime >= 1000) {
        *puiCurrentTick = !(*puiCurrentTick);
        if (puiCurrentTick++ == uiTicks + TICK_AMOUNT - 1)
            puiCurrentTick = uiTicks;

        uiSeconds++;
        if (uiSeconds >= 60) {
            uiMinute++;
            uiSeconds = 0;
        }

        if (uiMinute >= 60) {
            uiHour++;
            uiMinute = 0;
        }

        if (uiHour >= 24) {
            uiHour = 0;
        }

        uiPassedTime = 0;
    }
}

void DIGIVICE_passTime(pass_time_selector_t ePassTime) {
    switch (ePassTime) {
        case MINUTES:
            uiMinute++;
            uiLastMinute = uiMinute;
            if (uiMinute >= 60)
                uiMinute = 0;
            break;
        case HOUR:
            uiHour++;
            if (uiHour >= 24)
                uiHour = 0;
            break;
    }
}

void DIGIVICE_updateClock(uint16_t uiDeltaTime) {
    if (!fIsConfiguring) {
        updateNotConfigurin(uiDeltaTime);
    }
}

void DIGIVICE_renderClock() {
    const uint8_t fAfternoon = uiHour >= 12;

    DIGIVICE_drawNumber(fAfternoon ? uiHour - 12 : uiHour, 1, 1,
                        EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawText(":", 9, 1, EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawNumber(uiMinute, 13, 1, EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawNumber(uiSeconds, 22, 1, EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawText(fAfternoon ? "PM" : "AM", 1, 9, EFFECT_NONE);

    if (!fIsConfiguring) {
        uint8_t i = 0, x = 16;
        for (i = 0; i < TICK_AMOUNT; i++) {
            DIGIVICE_drawTile(uiTicks[i] ? TIC_ON_TILE : TIC_OFF_TILE, x, 8,
                              EFFECT_NONE);
            x += 3;
        }
    } else {
        DIGIVICE_drawText("SET", 16, 9, EFFECT_NONE);
    }
}
