#ifndef DIGIMON_H
#define DIGIMON_H

#include <stdint.h>

// Stats
#define MASK_HUNGER      0b00001111
#define MASK_STRENGTH    0b11110000
#define MASK_SICK        0b00000001
#define MASK_INJURIED    0b00000010
#define MASK_CALLED      0b00000100
#define MASK_DYING_STAGE 0b00100000
#define MASK_SLEEPING    0b01000000

// Getters
#define GET_HUNGER_VALUE(x)   ((MASK_HUNGER & x) >> 0)
#define GET_STRENGTH_VALUE(x) ((MASK_STRENGTH & x) >> 4)

// Setters
#define SET_HUNGER_VALUE(variable, value) \
    variable &= ~MASK_HUNGER;             \
    variable |= ((value & 0b00001111) << 0)
#define SET_STRENGTH_VALUE(variable, value) \
    variable &= ~MASK_STRENGTH;             \
    variable |= ((value & 0b00001111) << 4)

// Evolution progression
#define MASK_NEEDS_CARE_MISTAKES     0b00000001
#define MASK_NEEDS_TRAINING          0b00000010
#define MASK_NEEDS_OVERFEED          0b00000100
#define MASK_NEEDS_WIN_COUNT         0b00001000
#define MASK_NEEDS_SLEEP_DISTURBANCE 0b00010000

#define NEEDS_CARE_MISTAKES(variable) (variable & MASK_NEEDS_CARE_MISTAKES) != 0
#define NEEDS_TRAINING(variable)      (variable & MASK_NEEDS_TRAINING) != 0
#define NEEDS_OVERFEED(variable)      (variable & MASK_NEEDS_OVERFEED) != 0
#define NEEDS_WIN_COUNT(variable)     (variable & MASK_NEEDS_WIN_COUNT) != 0
#define NEEDS_SLEEP_DISTURBANCE(variable) \
    (variable & MASK_NEEDS_SLEEP_DISTURBANCE) != 0

#define GET_MIN_VALUE(variable) ((uint8_t)(variable >> 8))
#define GET_MAX_VALUE(variable) ((uint8_t)variable)

#define MAX_POSSIBLE_EVOLUTIONS 10

typedef struct digimon_t {
    char szName[50];
    uint8_t uiSlotPower;
    uint16_t uiTimeWakeUp;
    uint16_t uiTimeSleep;
    uint8_t uiAttribute;
    uint16_t uiNeededTimeEvolution;
    uint8_t uiStage;
    uint8_t uiVersion;

    uint8_t uiCountPossibleEvolutions;
    struct evolution_requirement_t*
        vstEvolutionRequirements[MAX_POSSIBLE_EVOLUTIONS];
} digimon_t;

typedef struct evolution_requirement_t {
    uint8_t uiProgressionNeeded;
    uint16_t uiCareMistakesCount;
    uint16_t uiTrainingCount;
    uint16_t uiOverfeedingCount;
    uint16_t uiSleepDisturbanceCount;
    uint16_t uiWinCount;
    uint16_t uiIndexEvolution;
} evolution_requirement_t;

typedef struct {
    digimon_t* pstCurrentDigimon;
    uint8_t uiHungerStrength;
    uint8_t uiStats;
    uint8_t uiWeight;

    uint8_t uiCareMistakesCount;
    uint8_t uiTrainingCount;
    uint8_t uiOverfeedingCount;
    uint8_t uiSleepDisturbanceCount;
    uint8_t uiBattleCount;
    uint8_t uiWinCount;

    uint16_t uiTimeSinceLastMeal;
    uint16_t uiTimeSinceLastTraining;
    uint16_t uiTimeSinceLastPoop;
    uint16_t uiTimeToEvolve;
    uint16_t uiIndexCurrentDigimon;
    uint8_t uiPoopCount;
    int8_t iTimeBeingCalled;
    uint16_t uiTimeSickOrInjured;
    uint8_t uiInjuredCount;
    uint8_t uiSickCount;
} playing_digimon_t;

uint8_t DIGI_evolveDigimon();

uint8_t DIGI_shouldEvolve();

uint8_t DIGI_feedDigimon(int16_t uiAmount);

uint8_t DIGI_stregthenDigimon(int16_t uiAmount, int8_t iWeightChange);

uint8_t DIGI_trainDigimon(uint8_t uiAmount);

uint8_t DIGI_healDigimon(uint8_t uiType);

uint8_t DIGI_putSleep(uint8_t uiSleepMode);

uint8_t DIGI_shouldSleep();

uint8_t DIGI_shouldWakeUp();

uint8_t DIGI_proccesCalling(uint8_t uiTimePassed);

uint8_t DIGI_setCalled();

void DIGI_addCareMistakes();

uint8_t DIGI_poop(uint8_t uiAmount);

void DIGI_cleanPoop();

uint8_t DIGI_shouldBeKilledOff();

#endif  // DIGIMON_H