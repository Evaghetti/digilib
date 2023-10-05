#ifndef RENDER_H
#define RENDER_H

#include "digitype.h"

#define LCD_SCREEN_WIDTH  32
#define LCD_SCREEN_HEIGHT 16

#define LCD_CENTER_SPRITE (LCD_SCREEN_WIDTH / 2 - TILE_WIDTH)

#define TILE_WIDTH  8
#define TILE_HEIGHT 8

#define EFFECT_NONE              (0)
#define EFFECT_REVERSE_TILE      (1 << 0)
#define EFFECT_REVERSE_COLOR     (1 << 1)
#define EFFECT_SHOW_LEADING_ZERO (1 << 2)

void DIGIVICE_drawTile(const uint8_t* puiTile, uint8_t x, uint8_t y,
                       const uint8_t uiEffects);

void DIGIVICE_drawSprite(const uint16_t* const puiSprite, uint8_t x, uint8_t y,
                         uint8_t uiEffects);

void DIGIVICE_drawText(const char* pszText, uint8_t x, uint8_t y,
                       uint8_t uiEffects);

void DIGIVICE_drawNumber(uint8_t uiNumber, uint8_t x, uint8_t y,
                         uint8_t uiEffects);

void DIGIVICE_drawPopup(const uint16_t* puiPopup);

#endif  // RENDER_H
