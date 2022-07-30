#include "digihardware.h"

#include "enums.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// TLV savefile
#define TLV_INDEX_CURRENT_DIGIMON    0x00
#define TLV_STATS                    0x01
#define TLV_HUNGER_STRENGTH          0x02
#define TLV_WEIGHT                   0x03
#define TLV_TRAINING_COUNT           0x04
#define TLV_OVERFEEDING_COUNT        0x05
#define TLV_SLEEP_DISTURBANCE_COUNT  0x06
#define TLV_BATTLE_COUNT             0x07
#define TLV_WIN_COUNT                0x08
#define TLV_CARE_MISTAKES_COUNT      0x09
#define TLV_TIME_SINCE_LAST_MEAL     0x0a
#define TLV_TIME_SINCE_LAST_TRAINING 0x0b
#define TLV_TIME_SINCE_LAST_POOP     0x0c
#define TLV_TIME_TO_EVOLVE           0x0d
#define TLV_POOP_COUNT               0x0e
#define TLV_TIME_BEING_CALLED        0x0f
#define TLV_TIME_SICK_OR_INJURED     0x10
#define TLV_INJURED_COUNT            0x11
#define TLV_SICK_COUNT               0x12
#define TLV_AGE                      0x13
#define TLV_TIMED_FLAGS              0x14

static uint16_t guiTime = 0xffff;

uint16_t DIGIHW_setTime() {
    time_t uiNow = time(NULL);
    struct tm* pstCurrentTime = localtime(&uiNow);

    guiTime = (uint16_t)(pstCurrentTime->tm_hour * 60 + pstCurrentTime->tm_min);
    return guiTime;
}

uint16_t DIGIHW_timeMinutes() {
    return guiTime;
}

void DIGIHW_addTime(uint16_t uiAddingAmount) {
    guiTime = (guiTime + uiAddingAmount) % 1440;
}

uint8_t DIGIHW_randomNumber() {
    srand(time(NULL));

    return rand() % 16 + 1;
}

struct tlv_data_t {
    uint8_t tag, size;
    void* value;
};

#define TLV_FIELD(tag, value) \
    { tag, sizeof(value), &value }

uint8_t DIGIHW_readDigimon(const char* szFileName,
                           playing_digimon_t* pstPlayingDigimon) {
    FILE* pstFileHandle = fopen(szFileName, "rb");
    if (pstFileHandle == NULL)
        return DIGI_RET_ERROR;

    struct tlv_data_t vstDataSave[] = {
        TLV_FIELD(TLV_HUNGER_STRENGTH, pstPlayingDigimon->uiHungerStrength),
        TLV_FIELD(TLV_STATS, pstPlayingDigimon->uiStats),
        TLV_FIELD(TLV_WEIGHT, pstPlayingDigimon->uiWeight),
        TLV_FIELD(TLV_CARE_MISTAKES_COUNT,
                  pstPlayingDigimon->uiCareMistakesCount),
        TLV_FIELD(TLV_TRAINING_COUNT, pstPlayingDigimon->uiTrainingCount),
        TLV_FIELD(TLV_OVERFEEDING_COUNT, pstPlayingDigimon->uiOverfeedingCount),
        TLV_FIELD(TLV_SLEEP_DISTURBANCE_COUNT,
                  pstPlayingDigimon->uiSleepDisturbanceCount),
        TLV_FIELD(TLV_BATTLE_COUNT, pstPlayingDigimon->uiBattleCount),
        TLV_FIELD(TLV_WIN_COUNT, pstPlayingDigimon->uiWinCount),
        TLV_FIELD(TLV_TIME_SINCE_LAST_MEAL,
                  pstPlayingDigimon->uiTimeSinceLastMeal),
        TLV_FIELD(TLV_TIME_SINCE_LAST_TRAINING,
                  pstPlayingDigimon->uiTimeSinceLastTraining),
        TLV_FIELD(TLV_TIME_SINCE_LAST_POOP,
                  pstPlayingDigimon->uiTimeSinceLastPoop),
        TLV_FIELD(TLV_TIME_TO_EVOLVE, pstPlayingDigimon->uiTimeToEvolve),
        TLV_FIELD(TLV_INDEX_CURRENT_DIGIMON,
                  pstPlayingDigimon->uiIndexCurrentDigimon),
        TLV_FIELD(TLV_POOP_COUNT, pstPlayingDigimon->uiPoopCount),
        TLV_FIELD(TLV_TIME_BEING_CALLED, pstPlayingDigimon->iTimeBeingCalled),
        TLV_FIELD(TLV_TIME_SICK_OR_INJURED,
                  pstPlayingDigimon->uiTimeSickOrInjured),
        TLV_FIELD(TLV_INJURED_COUNT, pstPlayingDigimon->uiInjuredCount),
        TLV_FIELD(TLV_SICK_COUNT, pstPlayingDigimon->uiSickCount),
        TLV_FIELD(TLV_AGE, pstPlayingDigimon->uiAge),
        TLV_FIELD(TLV_TIMED_FLAGS, pstPlayingDigimon->uiTimedFlags)};

    uint8_t currentTag, currentLen;
    while (fread(&currentTag, sizeof(currentTag), 1, pstFileHandle) == 1) {
        fread(&currentLen, sizeof(currentLen), 1, pstFileHandle);

        unsigned i;
        for (i = 0; i < sizeof(vstDataSave) / sizeof(vstDataSave[0]); i++) {
            if (vstDataSave[i].tag == currentTag) {
                fread(vstDataSave[i].value, currentLen, 1, pstFileHandle);
                break;
            }
        }

        if (i == sizeof(vstDataSave) / sizeof(vstDataSave[0])) {
            fclose(pstFileHandle);
            return DIGI_RET_ERROR;
        }
    }

    fclose(pstFileHandle);
    return DIGI_RET_OK;
}

uint8_t DIGIHW_saveDigimon(const char* szFileName,
                           playing_digimon_t* pstPlayingDigimon) {
    FILE* pstFileHandle = fopen(szFileName, "wb");
    if (pstFileHandle == NULL)
        return DIGI_RET_ERROR;

    struct tlv_data_t vstDataSave[] = {
        TLV_FIELD(TLV_HUNGER_STRENGTH, pstPlayingDigimon->uiHungerStrength),
        TLV_FIELD(TLV_STATS, pstPlayingDigimon->uiStats),
        TLV_FIELD(TLV_WEIGHT, pstPlayingDigimon->uiWeight),
        TLV_FIELD(TLV_CARE_MISTAKES_COUNT,
                  pstPlayingDigimon->uiCareMistakesCount),
        TLV_FIELD(TLV_TRAINING_COUNT, pstPlayingDigimon->uiTrainingCount),
        TLV_FIELD(TLV_OVERFEEDING_COUNT, pstPlayingDigimon->uiOverfeedingCount),
        TLV_FIELD(TLV_SLEEP_DISTURBANCE_COUNT,
                  pstPlayingDigimon->uiSleepDisturbanceCount),
        TLV_FIELD(TLV_BATTLE_COUNT, pstPlayingDigimon->uiBattleCount),
        TLV_FIELD(TLV_WIN_COUNT, pstPlayingDigimon->uiWinCount),
        TLV_FIELD(TLV_TIME_SINCE_LAST_MEAL,
                  pstPlayingDigimon->uiTimeSinceLastMeal),
        TLV_FIELD(TLV_TIME_SINCE_LAST_TRAINING,
                  pstPlayingDigimon->uiTimeSinceLastTraining),
        TLV_FIELD(TLV_TIME_SINCE_LAST_POOP,
                  pstPlayingDigimon->uiTimeSinceLastPoop),
        TLV_FIELD(TLV_TIME_TO_EVOLVE, pstPlayingDigimon->uiTimeToEvolve),
        TLV_FIELD(TLV_INDEX_CURRENT_DIGIMON,
                  pstPlayingDigimon->uiIndexCurrentDigimon),
        TLV_FIELD(TLV_POOP_COUNT, pstPlayingDigimon->uiPoopCount),
        TLV_FIELD(TLV_TIME_BEING_CALLED, pstPlayingDigimon->iTimeBeingCalled),
        TLV_FIELD(TLV_TIME_SICK_OR_INJURED,
                  pstPlayingDigimon->uiTimeSickOrInjured),
        TLV_FIELD(TLV_INJURED_COUNT, pstPlayingDigimon->uiInjuredCount),
        TLV_FIELD(TLV_SICK_COUNT, pstPlayingDigimon->uiSickCount),
        TLV_FIELD(TLV_AGE, pstPlayingDigimon->uiAge),
        TLV_FIELD(TLV_TIMED_FLAGS, pstPlayingDigimon->uiTimedFlags)};

    unsigned i;
    for (i = 0; i < sizeof(vstDataSave) / sizeof(vstDataSave[0]); i++) {
        fwrite(&vstDataSave[i].tag, sizeof(vstDataSave[i].tag), 1,
               pstFileHandle);
        fwrite(&vstDataSave[i].size, sizeof(vstDataSave[i].size), 1,
               pstFileHandle);
        fwrite(vstDataSave[i].value, vstDataSave[i].size, 1, pstFileHandle);
    }
    fclose(pstFileHandle);
    return DIGI_RET_OK;
}