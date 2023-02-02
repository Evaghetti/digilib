#include "digivice.h"

#include "digiapi.h"
#include "enums.h"

#include "animation.h"
#include "render.h"
#include "sprites.h"

#define ONE_MINUTE 60000

static playing_digimon_t* gpstPlayingDigimon;

static size_t guiFrequency;

animation_t animation;

uint8_t DIGIVICE_init(const digihal_t* pstHal,
                      const digivice_hal_t* pstDigiviceHal,
                      size_t uiFrequency) {
    uint8_t uiRet = DIGI_init(pstHal, &gpstPlayingDigimon);
    if (uiRet == DIGI_RET_CHOOSE_DIGITAMA)
        uiRet = DIGI_selectDigitama(gpstPlayingDigimon, 0);
    
    if (uiRet)
        return uiRet;

    gpstDigiviceHal = pstDigiviceHal;
    guiFrequency = uiFrequency;
    animation.puiCurrentAnimation =
        guiDigimonWalkingAnimationDatabase[gpstPlayingDigimon
                                               ->uiIndexCurrentDigimon];
    animation.uiMaxFrameCount = 2;
    return uiRet;
}

static uint32_t getDeltaTime() {
    static size_t uiLastTime = 0;
    size_t uiCurrentTime = gpstDigiviceHal->getTimeStamp();
    if (uiLastTime == 0)
        uiLastTime = uiCurrentTime;
    uint32_t uiDeltaTime = uiCurrentTime - uiLastTime;
    uiLastTime = uiCurrentTime;
    return uiDeltaTime;
}

uint8_t DIGIVICE_update() {
    static uint32_t uiTimePassed = 0;

    uint32_t uiDeltaTime = getDeltaTime();
    uint8_t uiRet = DIGI_RET_OK, uiEvents;
    
    uiTimePassed += uiDeltaTime;
    if (uiTimePassed >= ONE_MINUTE) {
        uiRet = DIGI_updateEventsDeltaTime(gpstPlayingDigimon, 1, &uiEvents);
        uiTimePassed = 0;
    }

    DIGIVICE_updateAnimation(&animation, uiDeltaTime);

    DIGIVICE_drawSprite(DIGIVICE_getCurrentSpriteAnimation(&animation), 8, 0,
                        0);

    gpstDigiviceHal->render();
    return uiRet;
}
