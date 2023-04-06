#include "digivice.h"

#include "digiapi.h"
#include "enums.h"
#include "enums_digivice.h"

#include "menu.h"
#include "player.h"
#include "render.h"

static player_t stPlayer;
static menu_t gstMenu;

static uint8_t uiCurrentControllerState;
static uint8_t uiPreviousControllerState;
static int8_t uiCurrentIcon = -1;

static size_t guiFrequency;

static const menu_item_t gstMenuItemsFeed[] = {
    {.eType = MENU_ITEM_TEXT, .pDataItem = "Feed"},
    {.eType = MENU_ITEM_TEXT, .pDataItem = "Vitamin"}};

static const menu_item_t gstMenuItemsLights[] = {
    {.eType = MENU_ITEM_TEXT, .pDataItem = "ON"},
    {.eType = MENU_ITEM_TEXT, .pDataItem = "OFF"}};

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

static void handleButtonsPlayerState() {
    switch (stPlayer.eState) {
        case WALKING:
        case NEED_SLEEP:
        case SLEEPING:
            if (DIGIVICE_isButtonPressed(BUTTON_A)) {
                if (uiCurrentIcon >= 0)
                    gpstDigiviceHal->setIconStatus(uiCurrentIcon, 0);

                uiCurrentIcon = (uiCurrentIcon + 1) & 7;
                gpstDigiviceHal->setIconStatus(uiCurrentIcon, 1);
            }
            if (DIGIVICE_isButtonPressed(BUTTON_B)) {
                switch (uiCurrentIcon) {
                    case 1:
                        DIGIVICE_initMenu(&gstMenu,
                                          sizeof(gstMenuItemsFeed) /
                                              sizeof(gstMenuItemsFeed[0]),
                                          gstMenuItemsFeed);
                        break;
                    case 5:
                        DIGIVICE_initMenu(&gstMenu,
                                          sizeof(gstMenuItemsLights) /
                                              sizeof(gstMenuItemsLights[0]),
                                          gstMenuItemsLights);
                        break;
                    default:
                        break;
                }
            }
            break;
        case EATING:
        case EATING_VITAMIN:
            if (DIGIVICE_isButtonPressed(BUTTON_B)) {
                DIGIVICE_changeStatePlayer(&stPlayer, WALKING);
                gstMenu.fInUse = 1;
            }
            break;
        default:
            break;
    }
}

static void handleButtonsMenu() {
    if (DIGIVICE_isButtonPressed(BUTTON_A)) {
        DIGIVICE_advanceMenu(&gstMenu, MENU_DIRECTION_FORWARD);
    } else if (DIGIVICE_isButtonPressed(BUTTON_B)) {
        switch (uiCurrentIcon) {
            case 1:
                DIGIVICE_changeStatePlayer(&stPlayer,
                                           EATING + gstMenu.uiCurrentIndex);
                break;
            case 5:
                DIGIVICE_changeStatePlayer(
                    &stPlayer,
                    gstMenu.uiCurrentIndex == 0 ? WALKING : SLEEPING);
                break;
            default:
                break;
        }

        gstMenu.fInUse = 0;
    } else if (DIGIVICE_isButtonPressed(BUTTON_C)) {
        gstMenu.uiCountItems = 0;
        gstMenu.fInUse = 0;
    }
}

uint8_t DIGIVICE_update() {
    uint32_t uiDeltaTime = getDeltaTime();

    if (DIGIVICE_isMenuInUse(&gstMenu))
        handleButtonsMenu();
    else
        handleButtonsPlayerState();

    uint8_t uiCalling = (stPlayer.pstPet->uiStats & MASK_CALLED) >> 2;
    if (stPlayer.eState > HATCHING)
        gpstDigiviceHal->setIconStatus(CALL, uiCalling);

    uiPreviousControllerState = uiCurrentControllerState;

    uint8_t uiRet = DIGIVICE_updatePlayer(&stPlayer, uiDeltaTime);
    if (uiRet & DIGIVICE_EVENT_HAPPENED) {
        gpstDigiviceHal->setIconStatus(uiCurrentIcon, 0);
        uiCurrentIcon = -1;
    }
    if (uiRet & DIGIVICE_CHANGED_STATE)
        gstMenu.fInUse = 1;

    if (DIGIVICE_isMenuInUse(&gstMenu))
        DIGIVICE_drawMenu(&gstMenu);
    else
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