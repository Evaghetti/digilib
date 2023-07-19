#include "battle.h"

#include "digibattle_classic.h"
#include "digivice.h"
#include "digivice_hal.h"
#include "enums_digivice.h"
#include "render.h"
#include "sprites.h"

uint8_t DIGIVICE_canBattle(const player_t* pstPlayer) {
    return DIGIBATTLE_canBattle(pstPlayer->pstPet);
}

uint8_t DIGIVICE_tryBattle(player_t* pstPlayer) {
    uint8_t uiRet = DIGIVICE_canBattle(pstPlayer);

    if (uiRet != DIGIBATTLE_RET_OK) {
        LOG("No longer can battle -> %d", uiRet);
        return DIGIVICE_CANCEL_BATTLE;
    }

    LOG("Going to start battle routines -> %d",
        DIGIVICE_isButtonPressed(BUTTON_B));
    uiRet = (DIGIVICE_isButtonPressed(BUTTON_B)
                 ? DIGIBATTLE_initiate(pstPlayer->pstPet)
                 : DIGIBATTLE_continue(pstPlayer->pstPet));
    switch (uiRet) {
        case DIGIBATTLE_RET_POLL:
            LOG("No data polled");
            return uiRet;
        case DIGIBATTLE_RET_ERROR:
            LOG("Error during battle -> %d", uiRet);
            return uiRet;
    }

    LOG("Finished battle, result -> %d", uiRet);
    DIGIBATTLE_changeStats(pstPlayer->pstPet, uiRet);
    return uiRet;
}

void DIGIVICE_renderBattleBanner() {
    DIGIVICE_drawPopup(guiBattlePopupSprite);
}
