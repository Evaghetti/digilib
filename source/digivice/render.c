#include "render.h"

#include "digivice_hal.h"
#include "sprites.h"

void DIGIVICE_drawTile(const uint8_t* puiTile, uint8_t x, uint8_t y,
                       const uint8_t uiInverted) {
    uint8_t i, j;

    for (i = y; i < y + 8 && i < 16; i++) {
        uint8_t uiCurrentLineTile = *puiTile;
        puiTile++;

        if (i < 0)
            continue;

        if (uiInverted) {
            uiCurrentLineTile = (uiCurrentLineTile & 0xF0) >> 4 |
                                (uiCurrentLineTile & 0x0F) << 4;
            uiCurrentLineTile = (uiCurrentLineTile & 0xCC) >> 2 |
                                (uiCurrentLineTile & 0x33) << 2;
            uiCurrentLineTile = (uiCurrentLineTile & 0xAA) >> 1 |
                                (uiCurrentLineTile & 0x55) << 1;
        }

        for (j = x; j < x + 8 && j < 32; j++) {
            uint8_t uiIsSet = uiCurrentLineTile & 0b10000000;
            uiCurrentLineTile <<= 1;
            if (j < 0)
                continue;

            gpstDigiviceHal->setLCDStatus(j, i, uiIsSet);
        }
    }
}

void DIGIVICE_drawSprite(const uint16_t* const puiSprite, uint8_t x, uint8_t y,
                         uint8_t uiInverted) {
    uint8_t xLeft = uiInverted ? x + 8 : x;
    uint8_t xRight = uiInverted ? x : x + 8;

    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite))], xLeft, y,
                      IS_TILE_INVERTED((*(puiSprite))) ^ uiInverted);
    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite + 1))],
                      xRight, y,
                      IS_TILE_INVERTED((*(puiSprite + 1))) ^ uiInverted);
    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite + 2))], xLeft,
                      y + 8, IS_TILE_INVERTED((*(puiSprite + 2))) ^ uiInverted);
    DIGIVICE_drawTile(&guiTileDatabase[GET_INDEX_TILE(*(puiSprite + 3))],
                      xRight, y + 8,
                      IS_TILE_INVERTED((*(puiSprite + 3))) ^ uiInverted);
}

void DIGIVICE_drawText(const char* pszText, uint8_t x, uint8_t y,
                       uint8_t uiInverted) {
    while (*pszText != '\0') {
        const uint16_t uiIndex = ((*pszText) - FIRST_CHARACTER) << 3;
        DIGIVICE_drawTile(&guiFontDatabase[uiIndex], x, y, uiInverted);

        pszText++;
        x += 4;
        if (x >= LCD_SCREEN_WIDTH)
            break;
    }
}