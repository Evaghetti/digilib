#ifndef RENDER_H
#define RENDER_H

#include "digitype.h"

#define LCD_SCREEN_WIDTH  32
#define LCD_SCREEN_HEIGHT 16

#define LCD_CENTER_SPRITE (LCD_SCREEN_WIDTH / 2 - TILE_WIDTH)

#define TILE_WIDTH  8
#define TILE_HEIGHT 8

void DIGIVICE_drawTile(const uint8_t* puiTile, uint8_t x, uint8_t y,
                       const uint8_t uiInverted);

void DIGIVICE_drawSprite(const uint16_t* const puiSprite, uint8_t x, uint8_t y,
                         uint8_t uiInverted);

void DIGIVICE_drawText(const char* pszText, uint8_t x, uint8_t y,
                       uint8_t uiInverted);

#endif // RENDER_H