#include "digivice.h"

#include "digiapi.h"
#include "enums.h"
#include "enums_digivice.h"

#include "info.h"
#include "menu.h"
#include "player.h"
#include "render.h"
#include "training.h"

typedef enum game_state_e {
    PLAYER_STATE,
    MENU_STATE,
    INFO_STATE,
    TRAINING_STATE,
} game_state_e;

static player_t stPlayer;
static menu_t gstMenu;

static uint8_t uiCurrentControllerState;
static uint8_t uiPreviousControllerState;
static int8_t uiCurrentIcon = -1;

static size_t guiFrequency;

static game_state_e eCurrentState;

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
        case SICK:
            if (DIGIVICE_isButtonPressed(BUTTON_A)) {
                if (uiCurrentIcon >= 0)
                    gpstDigiviceHal->setIconStatus(uiCurrentIcon, 0);

                uiCurrentIcon = (uiCurrentIcon + 1) & 7;
                gpstDigiviceHal->setIconStatus(uiCurrentIcon, 1);
            }
            if (DIGIVICE_isButtonPressed(BUTTON_B)) {
                switch (uiCurrentIcon) {
                    case 0:
                        DIGIVICE_initInfoDisplay(stPlayer.pstPet);
                        eCurrentState = INFO_STATE;
                        break;
                    case 1:
                        DIGIVICE_initMenu(&gstMenu,
                                          sizeof(gstMenuItemsFeed) /
                                              sizeof(gstMenuItemsFeed[0]),
                                          gstMenuItemsFeed);
                        eCurrentState = MENU_STATE;
                        break;
                    case 2:
                        DIGIVICE_initTraining(stPlayer.pstPet);
                        eCurrentState = TRAINING_STATE;
                        break;
                    case 4:
                        DIGIVICE_changeStatePlayer(&stPlayer, CLEANING);
                        break;
                    case 5:
                        DIGIVICE_initMenu(&gstMenu,
                                          sizeof(gstMenuItemsLights) /
                                              sizeof(gstMenuItemsLights[0]),
                                          gstMenuItemsLights);
                        eCurrentState = MENU_STATE;
                        break;
                    case 6:
                        DIGIVICE_changeStatePlayer(&stPlayer, HEALING);
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
                eCurrentState = MENU_STATE;
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
        eCurrentState = PLAYER_STATE;
    } else if (DIGIVICE_isButtonPressed(BUTTON_C)) {
        gstMenu.uiCountItems = 0;
        gstMenu.fInUse = 0;
        eCurrentState = PLAYER_STATE;
    }
}

void handleInfoState() {
    if (DIGIVICE_isButtonPressed(BUTTON_A) ||
        DIGIVICE_isButtonPressed(BUTTON_B)) {
        DIGIVICE_advanceInfoDisplay(1);
    } else if (DIGIVICE_isButtonPressed(BUTTON_C)) {
        eCurrentState = PLAYER_STATE;
    }
}

uint8_t DIGIVICE_update() {
    uint32_t uiDeltaTime = getDeltaTime();
    uint8_t uiRet;

    switch (eCurrentState) {
        case PLAYER_STATE:
            handleButtonsPlayerState();
            break;
        case MENU_STATE:
            handleButtonsMenu();
            break;
        case INFO_STATE:
            handleInfoState();
            break;
        case TRAINING_STATE:
            DIGIVICE_handleInputTraining();
            break;
        default:
            break;
    }

    uint8_t uiCalling = (stPlayer.pstPet->uiStats & MASK_CALLED) >> 2;
    if (stPlayer.eState > HATCHING)
        gpstDigiviceHal->setIconStatus(CALL, uiCalling);

    uiPreviousControllerState = uiCurrentControllerState;

    switch (eCurrentState) {
        case PLAYER_STATE:
            uiRet = DIGIVICE_updatePlayer(&stPlayer, uiDeltaTime);
            if (uiRet & DIGIVICE_EVENT_HAPPENED) {
                gpstDigiviceHal->setIconStatus(uiCurrentIcon, 0);
                uiCurrentIcon = -1;
            }
            if (uiRet & DIGIVICE_CHANGED_STATE)
                gstMenu.fInUse = 1;

            DIGIVICE_renderPlayer(&stPlayer);
            break;
        case MENU_STATE:
            DIGIVICE_drawMenu(&gstMenu);
            break;
        case INFO_STATE:
            DIGIVICE_updateInfoDisplay(uiDeltaTime);
            DIGIVICE_renderInfoDisplay();
            break;
        case TRAINING_STATE:
            DIGIVICE_updateTraining(uiDeltaTime);
            DIGIVICE_renderTraining();
            break;
    }

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