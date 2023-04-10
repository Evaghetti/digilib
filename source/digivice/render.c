#include "render.h"

#include "digivice_hal.h"
#include "sprites.h"

void DIGIVICE_drawTile(const uint8_t* puiTile, uint8_t x, uint8_t y,
                       const uint8_t uiEffects) {
    uint8_t i, j;

    for (i = y; i < y + 8 && i < 16; i++) {
        uint8_t uiCurrentLineTile = *puiTile;
        puiTile++;

        if (i < 0)
            continue;

        if (uiEffects & EFFECT_REVERSE_TILE) {
            uiCurrentLineTile = (uiCurrentLineTile & 0xF0) >> 4 |
                                (uiCurrentLineTile & 0x0F) << 4;
            uiCurrentLineTile = (uiCurrentLineTile & 0xCC) >> 2 |
                                (uiCurrentLineTile & 0x33) << 2;
            uiCurrentLineTile = (uiCurrentLineTile & 0xAA) >> 1 |
                                (uiCurrentLineTile & 0x55) << 1;
        }

        if (uiEffects & EFFECT_REVERSE_COLOR)
            uiCurrentLineTile ^= 0xff;

        uint8_t uiCount = 8;
        for (j = x; uiCount; j++, uiCount--) {
            uint8_t uiIsSet = uiCurrentLineTile & 0b10000000;
            uiCurrentLineTile <<= 1;
            if (j >= LCD_SCREEN_WIDTH)
                continue;

            gpstDigiviceHal->setLCDStatus(j, i, uiIsSet);
        }
    }
}

void DIGIVICE_drawSprite(const uint16_t* const puiSprite, uint8_t x, uint8_t y,
                         uint8_t uiEffects) {
    uint8_t xLeft = uiEffects & EFFECT_REVERSE_TILE ? x + 8 : x;
    uint8_t xRight = uiEffects & EFFECT_REVERSE_TILE ? x : x + 8;

    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite))], xLeft, y,
                      IS_TILE_INVERTED((*(puiSprite))) ^ uiEffects);
    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite + 1))],
                      xRight, y,
                      IS_TILE_INVERTED((*(puiSprite + 1))) ^ uiEffects);
    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite + 2))], xLeft,
                      y + 8, IS_TILE_INVERTED((*(puiSprite + 2))) ^ uiEffects);
    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite + 3))],
                      xRight, y + 8,
                      IS_TILE_INVERTED((*(puiSprite + 3))) ^ uiEffects);
}

void DIGIVICE_drawText(const char* pszText, uint8_t x, uint8_t y,
                       uint8_t uiEffects) {
    while (*pszText != '\0') {
        const uint16_t uiIndex = ((*pszText) - FIRST_CHARACTER) << 3;
        DIGIVICE_drawTile(&guiFontDatabase[uiIndex], x, y, uiEffects);

        pszText++;
        x += 4;
        if (x >= LCD_SCREEN_WIDTH)
            continue;
    }
}

void DIGIVICE_drawNumber(uint8_t uiNumber, uint8_t x, uint8_t y,
                         uint8_t uiEffects) {
    uint8_t uiShift = 0;
    uint8_t uiBcdResult = 0;

    if (uiNumber == 0) {
        DIGIVICE_drawText("0", x, y, uiEffects);
        return;
    }

    while (uiNumber > 0) {
        uiBcdResult |= (uiNumber % 10) << (uiShift++ << 2);
        uiNumber /= 10;
    }

    uiBcdResult = ((uiBcdResult & 0x0F) << 4 | (uiBcdResult & 0xF0) >> 4);
    if (uiBcdResult & 0b11110000)
        x -= 4;

    while (uiBcdResult) {
        uint8_t uiCurrentNumber = uiBcdResult & 0b00001111;
        if (uiCurrentNumber) {
            const uint16_t uiIndex = (('0' + uiCurrentNumber) - FIRST_CHARACTER)
                                     << 3;

            DIGIVICE_drawTile(&guiFontDatabase[uiIndex], x, y, uiEffects);
            x += 4;
        }

        uiBcdResult >>= 4;
    }
}