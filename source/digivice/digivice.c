#include "digivice.h"

#include "digiapi.h"
#include "enums.h"

#include "render.h"
#include "player.h"

static player_t stPlayer;

static size_t guiFrequency;

uint8_t DIGIVICE_init(const digihal_t* pstHal,
                      const digivice_hal_t* pstDigiviceHal,
                      size_t uiFrequency) {
    uint8_t uiRet = DIGI_init(pstHal, &stPlayer.pstPet);
    if (uiRet == DIGI_RET_CHOOSE_DIGITAMA)
        uiRet = DIGI_selectDigitama(stPlayer.pstPet, 0);

    if (uiRet) {
        LOG("Error initializing digilib -> %d", uiRet);
        return uiRet;
    }

    uiRet = DIGIVICE_initPlayer(&stPlayer);
    if (uiRet) {
        LOG("Error initializing player -> %d", uiRet);
        return uiRet;
    }

    gpstDigiviceHal = pstDigiviceHal;
    guiFrequency = uiFrequency;
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
    uint32_t uiDeltaTime = getDeltaTime();
    DIGIVICE_updatePlayer(&stPlayer, uiDeltaTime);
    DIGIVICE_renderPlayer(&stPlayer);
    gpstDigiviceHal->render();
    return DIGI_RET_OK;
}
