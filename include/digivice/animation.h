#ifndef ANIMATION_H
#define ANIMATION_H

#include "digitype.h"

typedef struct animation_t {
    const uint16_t* const* puiCurrentAnimation;
    uint8_t uiCurrentFrame, uiMaxFrameCount;
    uint16_t uiTimePassed;
} animation_t;

void DIGIVICE_setCurrentAnimation(animation_t* pstAnimation,
                                  const uint16_t* const* puiNewAnimation,
                                  uint8_t uiMaxFrameCount);

void DIGIVICE_updateAnimation(animation_t* pstAnimation, uint16_t uiDeltaTime);

const uint16_t* const DIGIVICE_getCurrentSpriteAnimation(
    const animation_t* pstAnimation);

#endif  // ANIMATION_H