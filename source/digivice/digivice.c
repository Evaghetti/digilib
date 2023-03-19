#include "digivice.h"

#include "digiapi.h"
#include "enums.h"
#include "enums_digivice.h"

#include "render.h"
#include "player.h"

static player_t stPlayer;

static uint8_t uiCurrentControllerState;
static uint8_t uiPreviousControllerState;
static int8_t uiCurrentIcon = -1;

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

    switch (stPlayer.eState)
    {
    case WALKING:
        if (DIGIVICE_isButtonPressed(BUTTON_A)) {
            if (uiCurrentIcon >= 0)
                gpstDigiviceHal->setIconStatus(uiCurrentIcon, 0);

            uiCurrentIcon = (uiCurrentIcon + 1) & 7;
            gpstDigiviceHal->setIconStatus(uiCurrentIcon, 1);
        }
        break;
    
    default:
        break;
    }

    uint8_t uiCalling = (stPlayer.pstPet->uiStats & MASK_CALLED) >> 2;
    if (stPlayer.eState > HATCHING)
        gpstDigiviceHal->setIconStatus(CALL, uiCalling);

    uiPreviousControllerState = uiCurrentControllerState;

    uint8_t uiRet = DIGIVICE_updatePlayer(&stPlayer, uiDeltaTime);
    switch (uiRet) {
    case DIGIVICE_CHANGED_STATE:
        gpstDigiviceHal->setIconStatus(uiCurrentIcon, 0);
        uiCurrentIcon = -1;
        break;

    default:
        break;
    }

    DIGIVICE_renderPlayer(&stPlayer);
    gpstDigiviceHal->render();
    return DIGI_RET_OK;
}

void DIGIVICE_setButtonState(digivice_buttons_e eButton, uint8_t uiState) {
    uiCurrentControllerState &= ~(1 << eButton);
    uiState <<= eButton;
    uiCurrentControllerState |= uiState;
}

uint8_t DIGIVICE_isButtonPressed(digivice_buttons_e eButton) {
    uint8_t uiBitToCheck = 1 << eButton;

    return (uiCurrentControllerState & uiBitToCheck) &&
           !(uiPreviousControllerState & uiBitToCheck);
}