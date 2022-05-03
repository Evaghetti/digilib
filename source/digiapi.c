#include "digiapi.h"

#include "enums.h"

#include <stdio.h>

#define TIME_TO_GET_HUNGRY 5
#define TIME_TO_GET_WEAKER 10
#define TIME_TO_POOP       15

extern digimon_t vstPossibleDigimon[];

static uint8_t guiAmountPoop = 0;
playing_digimon_t stPlayingDigimon;

int DIGI_init(const char* szSaveFile) {
    stPlayingDigimon.pstCurrentDigimon = &vstPossibleDigimon[0];
}

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents) {
    *puiEvents = 0;

    if (stPlayingDigimon.pstCurrentDigimon->uiStage >= DIGI_STAGE_BABY_1) {
        stPlayingDigimon.uiTimeSinceLastMeal += uiDeltaTime;
        stPlayingDigimon.uiTimeSinceLastTraining += uiDeltaTime;
        stPlayingDigimon.uiTimeSinceLastPoop += uiDeltaTime;
    }

    stPlayingDigimon.uiTimeToEvolve += uiDeltaTime;

    while (stPlayingDigimon.uiTimeSinceLastMeal >= TIME_TO_GET_HUNGRY) {
        uint8_t iRet = DIGI_feedDigimon(-1);
        if (iRet == DIGI_RET_HUNGRY)
            *puiEvents |= DIGI_EVENT_MASK_CALL;

        stPlayingDigimon.uiTimeSinceLastMeal -= TIME_TO_GET_HUNGRY;
    }

    while (stPlayingDigimon.uiTimeSinceLastTraining >= TIME_TO_GET_WEAKER) {
        uint8_t iRet = DIGI_stregthenDigimon(-1);
        if (iRet == DIGI_RET_WEAK)
            *puiEvents |= DIGI_EVENT_MASK_CALL;

        stPlayingDigimon.uiTimeSinceLastTraining -= TIME_TO_GET_WEAKER;
    }

    while (stPlayingDigimon.uiTimeSinceLastPoop >= TIME_TO_POOP &&
           guiAmountPoop < 4) {
        guiAmountPoop++;

        stPlayingDigimon.uiTimeSinceLastPoop -= TIME_TO_POOP;

        printf("[DIGILIB] %s has pooped!\n",
               stPlayingDigimon.pstCurrentDigimon->szName);
    }

    if (guiAmountPoop >= 4) {
        SET_SICK_VALUE(stPlayingDigimon.uiStats, 1);
        *puiEvents |= DIGI_EVENT_MASK_SICK;

        printf("[DIGILIB] %s got sick from all the waste around it.\n",
               stPlayingDigimon.pstCurrentDigimon->szName);
    }

    if (stPlayingDigimon.uiTimeToEvolve >=
        stPlayingDigimon.pstCurrentDigimon->uiNeededTimeEvolution) {
        uint8_t uiResult = DIGI_evolveDigimon();

        if (uiResult == DIGI_NO_EVOLUTION) {
            SET_DYING_VALUE(stPlayingDigimon.uiStats, 1);
            *puiEvents |= DIGI_EVENT_MASK_DIE;
        } else {
            *puiEvents |= DIGI_EVENT_MASK_EVOLVE;
        }
    }

    return DIGI_RET_OK;
}

void DIGI_cleanWaste() {
    printf("[DIGILIB] Cleaning %d poops\n", guiAmountPoop);
    guiAmountPoop = 0;
}
