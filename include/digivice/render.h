#ifndef RENDER_H
#define RENDER_H

#include "digitype.h"

void DIGIVICE_drawTile(const uint8_t* puiTile, uint8_t x, uint8_t y,
                       const uint8_t uiInverted);

void DIGIVICE_drawSprite(const uint16_t* const puiSprite, uint8_t x, uint8_t y,
                         uint8_t uiInverted);

#endif // RENDER_H