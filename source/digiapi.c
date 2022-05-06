#include "digiapi.h"

#include "digihardware.h"
#include "enums.h"

#include <stdio.h>
#include <stdlib.h>

#define TIME_TO_GET_HUNGRY 5
#define TIME_TO_GET_WEAKER 10
#define TIME_TO_POOP       15

#define TIME_TO_GET_CARE_MISTAKE      10
#define NOT_COUNTING_FOR_CARE_MISTAKE (TIME_TO_GET_CARE_MISTAKE + 1)
extern digimon_t vstPossibleDigimon[];

static int16_t guiTimeBeingCalled = NOT_COUNTING_FOR_CARE_MISTAKE;
static uint8_t guiAmountPoop = 0;
playing_digimon_t stPlayingDigimon;

int DIGI_init(const char* szSaveFile) {
    char szRealFileName[24] = {0};
    snprintf(szRealFileName, sizeof(szRealFileName), "%s.mon", szSaveFile);

    if (DIGIHW_readFile(szRealFileName, &stPlayingDigimon,
                        sizeof(stPlayingDigimon)) <= 0) {

        stPlayingDigimon.pstCurrentDigimon = &vstPossibleDigimon[3];
        SET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength, 4);
        SET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength, 4);

        DIGIHW_saveFile(szRealFileName, &stPlayingDigimon,
                        sizeof(stPlayingDigimon));
    }
    DIGIHW_setTime();
}

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents) {
    uint16_t uiCurrentTime = DIGIHW_timeMinutes();
    *puiEvents = 0;

    printf("[DIGILIB] Current Time: %d:%d\n", uiCurrentTime / 60,
           uiCurrentTime % 60);

    printf("%s\nHunger: %d\nStrength: %d\nCare Mistakes: %d\n",
           stPlayingDigimon.pstCurrentDigimon->szName,
           GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength),
           GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength),
           stPlayingDigimon.uiCareMistakesCount);

    if (stPlayingDigimon.pstCurrentDigimon->uiStage >= DIGI_STAGE_BABY_1) {
        stPlayingDigimon.uiTimeSinceLastMeal += uiDeltaTime;
        stPlayingDigimon.uiTimeSinceLastTraining += uiDeltaTime;
        stPlayingDigimon.uiTimeSinceLastPoop += uiDeltaTime;
    }

    stPlayingDigimon.uiTimeToEvolve += uiDeltaTime;

    while (stPlayingDigimon.uiTimeSinceLastMeal >= TIME_TO_GET_HUNGRY) {
        DIGI_feedDigimon(-1);

        stPlayingDigimon.uiTimeSinceLastMeal -= TIME_TO_GET_HUNGRY;
    }

    while (stPlayingDigimon.uiTimeSinceLastTraining >= TIME_TO_GET_WEAKER) {
        DIGI_stregthenDigimon(-1);

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

    if (DIGI_shouldEvolve() == DIGI_RET_OK) {
        uint8_t uiResult = DIGI_evolveDigimon();

        if (uiResult == DIGI_NO_EVOLUTION) {
            *puiEvents |= DIGI_EVENT_MASK_DIE;
        } else {
            *puiEvents |= DIGI_EVENT_MASK_EVOLVE;
        }
    }

    if (DIGI_shouldSleep() == DIGI_RET_OK) {
        printf("[DIGILIB] Bedtime for digimon\n");
        *puiEvents |= DIGI_EVENT_MASK_SLEEPY;
    }

    if (DIGI_setCalled() == DIGI_RET_OK) {
        *puiEvents |= DIGI_EVENT_MASK_CALL;

        if (guiTimeBeingCalled == NOT_COUNTING_FOR_CARE_MISTAKE)
            guiTimeBeingCalled = TIME_TO_GET_CARE_MISTAKE;
        else if (guiTimeBeingCalled > 0) {
            guiTimeBeingCalled -= uiDeltaTime;

            if (guiTimeBeingCalled <= 0)
                DIGI_addCareMistakes();
        }

        if (guiTimeBeingCalled <= 0)
            *puiEvents &= ~DIGI_EVENT_MASK_CALL;
    } else
        guiTimeBeingCalled = NOT_COUNTING_FOR_CARE_MISTAKE;

    DIGIHW_addTime(uiDeltaTime);
    return DIGI_RET_OK;
}

void DIGI_cleanWaste() {
    printf("[DIGILIB] Cleaning %d poops\n", guiAmountPoop);
    guiAmountPoop = 0;
}
