#include "digiapi.h"

#include "digihardware.h"
#include "digiworld.h"
#include "enums.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "logging.h"

#ifndef TIME_TO_GET_HUNGRY
#define TIME_TO_GET_HUNGRY 5
#endif

#ifndef TIME_TO_GET_WEAKER
#define TIME_TO_GET_WEAKER 10
#endif

#ifndef TIME_TO_POOP
#define TIME_TO_POOP 15
#endif

const char* gszSaveFile = NULL;
playing_digimon_t stPlayingDigimon;

uint8_t DIGI_init(const char* szSaveFile) {
    gszSaveFile = szSaveFile;

    // If no save file exists, needs to choose a digitama
    if (DIGIHW_readDigimon(gszSaveFile, &stPlayingDigimon) != DIGI_RET_OK)
        return DIGI_RET_ERROR;

    // If true, it means a previous digimon has died, need to select a
    // new digitama
    if (stPlayingDigimon.uiIndexCurrentDigimon >= MAX_COUNT_DIGIMON)
        return DIGI_RET_ERROR;

    stPlayingDigimon.pstCurrentDigimon =
        &vstPossibleDigimon[stPlayingDigimon.uiIndexCurrentDigimon];

    LOG("[DIGILIB] Current digimon: %s\n",
        stPlayingDigimon.pstCurrentDigimon->szName);

    DIGI_saveGame();
    DIGIHW_setTime();
    return DIGI_RET_OK;
}

uint8_t DIGI_initDigitama(const char* szSaveFile, uint8_t uiDigitamaIndex) {
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

    memset(&stPlayingDigimon, 0, sizeof(stPlayingDigimon));
    stPlayingDigimon.uiIndexCurrentDigimon = i;
    gszSaveFile = szSaveFile;
    DIGI_saveGame();
    return DIGI_init(szSaveFile);
}

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents) {
    uint16_t uiCurrentTime = DIGIHW_timeMinutes();
    uint16_t uiIsDying = (stPlayingDigimon.uiStats & MASK_DYING_STAGE);
    *puiEvents = 0;

    printf("%s\nHunger: %d\nStrength: %d\nCare Mistakes: %d\n",
           stPlayingDigimon.pstCurrentDigimon->szName,
           GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength),
           GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength),
           stPlayingDigimon.uiCareMistakesCount);

    if (stPlayingDigimon.pstCurrentDigimon->uiStage >= DIGI_STAGE_BABY_1 &&
        (stPlayingDigimon.uiStats & MASK_SLEEPING) == 0) {
        // If dying, then hearts and deplete twice as fast
        const uint16_t uiAlteredDeltaTime = uiDeltaTime << uiIsDying;

        stPlayingDigimon.uiTimeSinceLastMeal += uiAlteredDeltaTime;
        stPlayingDigimon.uiTimeSinceLastTraining += uiAlteredDeltaTime;
        stPlayingDigimon.uiTimeSinceLastPoop += uiAlteredDeltaTime;

        if ((stPlayingDigimon.uiStats & MASK_SICK) ||
            (stPlayingDigimon.uiStats & MASK_INJURIED))
            stPlayingDigimon.uiTimeSickOrInjured += uiDeltaTime;
    }

    if (!uiIsDying)
        stPlayingDigimon.uiTimeToEvolve += uiDeltaTime;

    if (DIGI_shouldBeKilledOff() == DIGI_RET_OK) {
        stPlayingDigimon.uiIndexCurrentDigimon = MAX_COUNT_DIGIMON;

        DIGIHW_addTime(uiDeltaTime);
        DIGI_saveGame();
        return DIGI_RET_DIED;
    }

    while (stPlayingDigimon.uiTimeSinceLastMeal >= TIME_TO_GET_HUNGRY) {
        DIGI_feedDigimon(-1);

        stPlayingDigimon.uiTimeSinceLastMeal -= TIME_TO_GET_HUNGRY;
    }

    while (stPlayingDigimon.uiTimeSinceLastTraining >= TIME_TO_GET_WEAKER) {
        DIGI_stregthenDigimon(-1, 0);

        stPlayingDigimon.uiTimeSinceLastTraining -= TIME_TO_GET_WEAKER;
    }

    while (stPlayingDigimon.uiTimeSinceLastPoop >= TIME_TO_POOP &&
           stPlayingDigimon.uiPoopCount < 4) {
        if (DIGI_poop(1) == DIGI_RET_SICK)
            *puiEvents |= DIGI_EVENT_MASK_SICK;
        *puiEvents |= DIGI_EVENT_MASK_POOP;

        stPlayingDigimon.uiTimeSinceLastPoop -= TIME_TO_POOP;
    }

    if (DIGI_shouldEvolve() == DIGI_RET_OK) {
        uint8_t uiResult = DIGI_evolveDigimon();

        if (uiResult == DIGI_NO_EVOLUTION) {
            *puiEvents |= DIGI_EVENT_MASK_DIE;
        } else {
            *puiEvents |= DIGI_EVENT_MASK_EVOLVE;
        }
    }

    if (DIGI_shouldSleep() == DIGI_RET_OK) {
        LOG("Bedtime for digimon");
        *puiEvents |= DIGI_EVENT_MASK_SLEEPY;
    } else if (DIGI_shouldWakeUp() == DIGI_RET_OK) {
        *puiEvents |= DIGI_EVENT_MASK_WOKE_UP;
        stPlayingDigimon.uiStats &= ~MASK_SLEEPING;
    }

    if (DIGI_setCalled() == DIGI_RET_OK) {
        *puiEvents |= DIGI_EVENT_MASK_CALL;
        if (DIGI_proccesCalling(uiDeltaTime) != DIGI_RET_OK)
            *puiEvents &= ~DIGI_EVENT_MASK_CALL;
    }

    DIGIHW_addTime(uiDeltaTime);
    if (uiCurrentTime + uiDeltaTime >= 1440 && stPlayingDigimon.uiAge < 99)
        stPlayingDigimon.uiAge++;

    // TODO: Find another way to save current digimon state
    // So the NULLing before saving isn't neccessary anymore.
    DIGI_saveGame();
    return DIGI_RET_OK;
}

playing_digimon_t DIGI_playingDigimon() {
    return stPlayingDigimon;
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

void DIGI_saveGame() {
    DIGIHW_saveDigimon(gszSaveFile, &stPlayingDigimon);
}