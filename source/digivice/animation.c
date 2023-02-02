#include "animation.h"

#include "render.h"

#define DEFAULT_HOLDTIME 500

void DIGIVICE_setCurrentAnimation(animation_t* pstAnimation,
                                  const uint16_t* const* puiNewAnimation,
                                  uint8_t uiMaxFrameCount) {
    pstAnimation->puiCurrentAnimation = puiNewAnimation;
    pstAnimation->uiCurrentFrame = 0;
    pstAnimation->uiTimePassed = 0;
    pstAnimation->uiMaxFrameCount = uiMaxFrameCount;
}

void DIGIVICE_updateAnimation(animation_t* pstAnimation, uint16_t uiDeltaTime) {
    pstAnimation->uiTimePassed += uiDeltaTime;

    if (pstAnimation->uiTimePassed >= DEFAULT_HOLDTIME) {
        pstAnimation->uiTimePassed = 0;

        pstAnimation->uiCurrentFrame++;
        if (pstAnimation->uiCurrentFrame >= pstAnimation->uiMaxFrameCount)
            pstAnimation->uiCurrentFrame = 0;
    }
}

const uint16_t* const DIGIVICE_getCurrentSpriteAnimation(
    const animation_t* pstAnimation) {
    return pstAnimation->puiCurrentAnimation[pstAnimation->uiCurrentFrame];
}