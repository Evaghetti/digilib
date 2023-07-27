#include "clock.h"
#include <stdint.h>
#include "render.h"
#include "sprites.h"

#define TICK_AMOUNT 5

static uint16_t uiPassedTime = 0;
static uint8_t uiHour = 0, uiMinute = 0, uiSeconds = 0;
static uint8_t uiTicks[TICK_AMOUNT] = {0}, *puiCurrentTick = uiTicks;

void DIGIVICE_updateClock(uint16_t uiDeltaTime, uint8_t fIsConfiguring) {
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

void DIGIVICE_renderClock() {
    const uint8_t fAfternoon = uiHour >= 12;

    DIGIVICE_drawNumber(fAfternoon ? uiHour - 12 : uiHour, 1, 1,
                        EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawText(":", 9, 1, EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawNumber(uiMinute, 13, 1, EFFECT_SHOW_LEADING_ZERO);
    DIGIVICE_drawNumber(uiSeconds, 22, 1, EFFECT_SHOW_LEADING_ZERO);

    DIGIVICE_drawText(fAfternoon ? "PM" : "AM", 1, 9, EFFECT_NONE);

    uint8_t i = 0, x = 16;
    for (i = 0; i < TICK_AMOUNT; i++) {
        DIGIVICE_drawTile(uiTicks[i] ? TIC_ON_TILE : TIC_OFF_TILE, x, 8,
                          EFFECT_NONE);
        x += 3;
    }
}
