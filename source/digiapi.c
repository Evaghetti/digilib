#include "digiapi.h"

#include "digihal.h"
#include "digiworld.h"
#include "enums.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define FPS  30
#define FREQ 1000000

uint32_t guiCurrentTime = 960;

int DIGI_init(const digihal_t* pstConfig,
              playing_digimon_t** pstPlayingDigimon) {
    if (DIGI_setHal(pstConfig) != DIGI_RET_OK) {
        LOG("Incorrect hal provided");
        return DIGI_RET_ERROR;
    }

    *pstPlayingDigimon = gpstHal->malloc(sizeof(playing_digimon_t));
    if (pstPlayingDigimon == NULL) {
        LOG("Error allocating digimon");
        return DIGI_RET_ERROR;
    }

    if (DIGI_initDigimon(*pstPlayingDigimon) != DIGI_RET_OK) {
        LOG("Error loading digimon");
        return DIGI_RET_CHOOSE_DIGITAMA;
    }

    LOG("Init %s - E: %d - HS: %04x CM: %d SD: %d TC: %d OF: %d",
        (*pstPlayingDigimon)->pstCurrentDigimon->szName,
        (*pstPlayingDigimon)->uiTimeToEvolve,
        (*pstPlayingDigimon)->uiHungerStrength,
        (*pstPlayingDigimon)->uiCareMistakesCount,
        (*pstPlayingDigimon)->uiSleepDisturbanceCount,
        (*pstPlayingDigimon)->uiTrainingCount,
        (*pstPlayingDigimon)->uiOverfeedingCount);
    return DIGI_RET_OK;
}

uint8_t DIGI_initDigimon(playing_digimon_t* pstPlayingDigimon) {
    // If no save file exists, needs to choose a digitama
    if (DIGI_readGame(pstPlayingDigimon) != DIGI_RET_OK)
        return DIGI_RET_CHOOSE_DIGITAMA;

    // If true, it means a previous digimon has died, need to select a
    // new digitama
    if (pstPlayingDigimon->uiIndexCurrentDigimon >= MAX_COUNT_DIGIMON)
        return DIGI_RET_CHOOSE_DIGITAMA;

    pstPlayingDigimon->pstCurrentDigimon =
        &vstPossibleDigimon[pstPlayingDigimon->uiIndexCurrentDigimon];
    DIGI_saveGame(pstPlayingDigimon);
    return DIGI_RET_OK;
}

uint8_t DIGI_selectDigitama(playing_digimon_t* pstPlayingDigimon,
                            uint8_t uiDigitamaIndex) {
    uint16_t i;
    uint8_t uiCountDigitama;

    digimon_t** pstPossibleDigitama = DIGI_possibleDigitama(&uiCountDigitama);
    if (uiDigitamaIndex >= uiCountDigitama)
        return DIGI_RET_ERROR;

    for (i = 0; i < MAX_COUNT_DIGIMON; i++) {
        if (memcmp(pstPossibleDigitama[uiDigitamaIndex], &vstPossibleDigimon[i],
                   sizeof(digimon_t)) == 0)
            break;
    }

    memset(pstPlayingDigimon, 0, sizeof(*pstPlayingDigimon));
    pstPlayingDigimon->uiIndexCurrentDigimon = i;
    pstPlayingDigimon->pstCurrentDigimon = &vstPossibleDigimon[i];
    pstPlayingDigimon->uiTimedFlags =
        DIGI_TIMEDFLG_CAN_OVERFEED | DIGI_TIMEDFLG_CAN_DISTURB_SLEEP;
    DIGI_saveGame(pstPlayingDigimon);
    return DIGI_RET_OK;
}

uint8_t DIGI_updateEventsDeltaTime(playing_digimon_t* pstPlayingDigimon,
                                   uint16_t uiDeltaTime, uint8_t* puiEvents) {
    uint16_t uiCurrentTime = guiCurrentTime;
    uint16_t uiIsDying = (pstPlayingDigimon->uiStats & MASK_DYING_STAGE);
    *puiEvents = 0;

    LOG("%s - E: %d - HS: %04x CM: %d SD: %d TC: %d OF: %d",
        pstPlayingDigimon->pstCurrentDigimon->szName,
        pstPlayingDigimon->uiTimeToEvolve, pstPlayingDigimon->uiHungerStrength,
        pstPlayingDigimon->uiCareMistakesCount,
        pstPlayingDigimon->uiSleepDisturbanceCount,
        pstPlayingDigimon->uiTrainingCount,
        pstPlayingDigimon->uiOverfeedingCount);

    if (pstPlayingDigimon->pstCurrentDigimon->uiStage >= DIGI_STAGE_BABY_1 &&
        (pstPlayingDigimon->uiStats & MASK_SLEEPING) == 0) {
        // If dying, then hearts and deplete twice as fast
        const uint16_t uiAlteredDeltaTime = uiDeltaTime << uiIsDying;

        pstPlayingDigimon->uiTimeSinceLastMeal += uiAlteredDeltaTime;
        pstPlayingDigimon->uiTimeSinceLastTraining += uiAlteredDeltaTime;
        pstPlayingDigimon->uiTimeSinceLastPoop += uiAlteredDeltaTime;

        if ((pstPlayingDigimon->uiStats & MASK_SICK) ||
            (pstPlayingDigimon->uiStats & MASK_INJURIED))
            pstPlayingDigimon->uiTimeSickOrInjured += uiDeltaTime;
    }

    if (!uiIsDying)
        pstPlayingDigimon->uiTimeToEvolve += uiDeltaTime;

    if (pstPlayingDigimon->pstCurrentDigimon->uiStage >= DIGI_STAGE_BABY_1) {
        if (DIGI_shouldBeKilledOff(pstPlayingDigimon) == DIGI_RET_OK) {
            pstPlayingDigimon->uiIndexCurrentDigimon = MAX_COUNT_DIGIMON;

            guiCurrentTime += uiDeltaTime;
            DIGI_saveGame(pstPlayingDigimon);
            return DIGI_RET_DIED;
        }

        while (pstPlayingDigimon->uiTimeSinceLastMeal >=
               DIGI_timeToGetHungry(pstPlayingDigimon)) {
            pstPlayingDigimon->uiTimeSinceLastMeal -=
                DIGI_timeToGetHungry(pstPlayingDigimon);

            DIGI_feedDigimon(pstPlayingDigimon, -1);
        }

        while (pstPlayingDigimon->uiTimeSinceLastTraining >=
               TIME_TO_GET_WEAKER) {
            pstPlayingDigimon->uiTimeSinceLastTraining -= TIME_TO_GET_WEAKER;

            DIGI_stregthenDigimon(pstPlayingDigimon, -1, 0);
        }

        while (pstPlayingDigimon->uiTimeSinceLastPoop >= TIME_TO_POOP &&
               pstPlayingDigimon->uiPoopCount < 4) {
            pstPlayingDigimon->uiTimeSinceLastPoop -= TIME_TO_POOP;

            if (DIGI_poop(pstPlayingDigimon, 1) == DIGI_RET_SICK)
                *puiEvents |= DIGI_EVENT_MASK_SICK;
            *puiEvents |= DIGI_EVENT_MASK_POOP;
        }
    }

    if (DIGI_shouldEvolve(pstPlayingDigimon) == DIGI_RET_OK) {
        uint8_t uiResult = DIGI_evolveDigimon(pstPlayingDigimon);

        if (uiResult == DIGI_NO_EVOLUTION) {
            *puiEvents |= DIGI_EVENT_MASK_DIE;
        } else {
            *puiEvents |= DIGI_EVENT_MASK_EVOLVE;
        }
    }

    if (DIGI_updateSleepDisturbance(pstPlayingDigimon, uiDeltaTime) ==
        DIGI_RET_OK) {
        LOG("Digimon was woken up during sleep, waiting for it to be able to "
            "sleep again");
    } else if (DIGI_shouldSleep(pstPlayingDigimon) == DIGI_RET_OK) {
        LOG("Bedtime for digimon");
        *puiEvents |= DIGI_EVENT_MASK_SLEEPY;
    } else if (DIGI_shouldWakeUp(pstPlayingDigimon) == DIGI_RET_OK) {
        *puiEvents |= DIGI_EVENT_MASK_WOKE_UP;
        pstPlayingDigimon->uiStats &= ~MASK_SLEEPING;
    }

    if (DIGI_setCalled(pstPlayingDigimon) == DIGI_RET_OK) {
        *puiEvents |= DIGI_EVENT_MASK_CALL;
        if (DIGI_proccesCalling(pstPlayingDigimon, uiDeltaTime) != DIGI_RET_OK)
            *puiEvents &= ~DIGI_EVENT_MASK_CALL;
    }

    guiCurrentTime += uiDeltaTime;
    if (uiCurrentTime + uiDeltaTime >= 1440 && pstPlayingDigimon->uiAge < 99)
        pstPlayingDigimon->uiAge++;
    DIGI_saveGame(pstPlayingDigimon);
    return DIGI_RET_OK;
}

digimon_t** DIGI_possibleDigitama(uint8_t* puiCount) {
    static digimon_t* vstDigitamas[MAX_COUNT_DIGIMON];
    static uint8_t uiCountEgg = 0;

    if (uiCountEgg == 0) {
        int i;

        for (i = 0; i < MAX_COUNT_DIGIMON; i++) {
            if (vstPossibleDigimon[i].uiStage == DIGI_STAGE_EGG)
                vstDigitamas[uiCountEgg++] = &vstPossibleDigimon[i];
        }
    }

    *puiCount = uiCountEgg;
    return vstDigitamas;
}

uint8_t DIGI_readGame(playing_digimon_t* pstPlayingDigimon) {
    if (gpstHal->readData == NULL)
        return DIGI_RET_ERROR;

    // TODO: TLV
    size_t iRet =
        gpstHal->readData(pstPlayingDigimon, sizeof(*pstPlayingDigimon));
    return iRet == sizeof(playing_digimon_t) ? DIGI_RET_OK : DIGI_RET_ERROR;
}

void DIGI_saveGame(playing_digimon_t* pstPlayingDigimon) {
    if (gpstHal->saveData == NULL)
        return;
    // TODO: TLV
    digimon_t* pstTemp = pstPlayingDigimon->pstCurrentDigimon;
    pstPlayingDigimon->pstCurrentDigimon = NULL;
    size_t iRet =
        gpstHal->saveData(pstPlayingDigimon, sizeof(*pstPlayingDigimon));
    pstPlayingDigimon->pstCurrentDigimon = pstTemp;
    if (iRet != sizeof(playing_digimon_t)) {
        LOG("Error saving game -> %d");
    }
}
