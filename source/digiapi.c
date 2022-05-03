#include "digiapi.h"

#include "enums.h"

#include <stdio.h>

#define TIME_TO_GET_HUNGRY 5
#define TIME_TO_GET_WEAKER 10
#define TIME_TO_POOP       15

static uint8_t guiAmountPoop = 3;

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents,
                                   playing_digimon_t* pstPlayingDigimon) {
    *puiEvents = 0;

    if (pstPlayingDigimon->pstCurrentDigimon->uiStage >= DIGI_STAGE_BABY_1) {
        pstPlayingDigimon->uiTimeSinceLastMeal += uiDeltaTime;
        pstPlayingDigimon->uiTimeSinceLastTraining += uiDeltaTime;
        pstPlayingDigimon->uiTimeSinceLastPoop += uiDeltaTime;
    }

    pstPlayingDigimon->uiTimeToEvolve += uiDeltaTime;

    while (pstPlayingDigimon->uiTimeSinceLastMeal >= TIME_TO_GET_HUNGRY) {
        uint8_t iRet = DIGI_feedDigimon(pstPlayingDigimon, -1);
        if (iRet == DIGI_RET_HUNGRY)
            *puiEvents |= DIGI_EVENT_MASK_CALL;

        pstPlayingDigimon->uiTimeSinceLastMeal -= TIME_TO_GET_HUNGRY;
    }

    while (pstPlayingDigimon->uiTimeSinceLastTraining >= TIME_TO_GET_WEAKER) {
        uint8_t iRet = DIGI_stregthenDigimon(pstPlayingDigimon, -1);
        if (iRet == DIGI_RET_WEAK)
            *puiEvents |= DIGI_EVENT_MASK_CALL;

        pstPlayingDigimon->uiTimeSinceLastTraining -= TIME_TO_GET_WEAKER;
    }

    while (pstPlayingDigimon->uiTimeSinceLastPoop >= TIME_TO_POOP &&
           guiAmountPoop < 4) {
        guiAmountPoop++;

        pstPlayingDigimon->uiTimeSinceLastPoop -= TIME_TO_POOP;

        printf("[DIGILIB] %s has pooped!\n",
               pstPlayingDigimon->pstCurrentDigimon->szName);
    }

    if (guiAmountPoop >= 4) {
        SET_SICK_VALUE(pstPlayingDigimon->uiStats, 1);
        *puiEvents |= DIGI_EVENT_MASK_SICK;

        printf("[DIGILIB] %s got sick from all the waste around it.\n",
               pstPlayingDigimon->pstCurrentDigimon->szName);
    }

    if (pstPlayingDigimon->uiTimeToEvolve >=
        pstPlayingDigimon->pstCurrentDigimon->uiNeededTimeEvolution) {
        uint8_t uiResult = DIGI_evolveDigimon(pstPlayingDigimon);

        if (uiResult == DIGI_NO_EVOLUTION) {
            SET_DYING_VALUE(pstPlayingDigimon->uiStats, 1);
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
