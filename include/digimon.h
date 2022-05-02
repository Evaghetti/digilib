#ifndef DIGIMON_H
#define DIGIMON_H

#include <stdint.h>

// Stats
#define MASK_HUNGER          0b00000011
#define MASK_STRENGTH        0b00001100
#define MASK_SICK            0b00010000
#define MASK_INJURIED        0b00100000
#define MASK_EVOLUTION_STAGE 0b11000000

// Getters
#define GET_HUNGER_VALUE(x)          ((MASK_HUNGER & x) >> 0)
#define GET_STRENGTH_VALUE(x)        ((MASK_STRENGTH & x) >> 2)
#define GET_SICK_VALUE(x)            ((MASK_SICK & x) >> 4)
#define GET_INJURIED_VALUE(x)        ((MASK_INJURIED & x) >> 5)
#define GET_EVOLUTION_STAGE_VALUE(x) ((MASK_EVOLUTION_STAGE & x) >> 6)

// Setters
#define SET_HUNGER_VALUE(variable, value) \
    variable &= ~MASK_HUNGER;             \
    variable |= ((value & 0b00000011) << 0)
#define SET_STRENGTH_VALUE(variable, value) \
    variable &= ~MASK_STRENGTH;             \
    variable |= ((value & 0b00000011) << 2)
#define SET_SICK_VALUE(variable, value) \
    variable &= ~MASK_SICK;             \
    variable |= ((value & 0b00000001) << 4)
#define SET_INJURIED_VALUE(variable, value) \
    variable &= ~MASK_INJURIED;             \
    variable |= ((value & 0b00000001) << 5)

// Evolution progression
#define MASK_CARE_MISTAKES   0b00000011
#define MASK_EFFORT          0b00001100
#define MASK_OVERFEED        0b00110000
#define MASK_NEEDS_OVERFEED  0b01000000
#define MASK_NEEDS_WIN_COUNT 0b10000000

// Getters
#define GET_CARE_MISTAKES_VALUE(x) ((MASK_CARE_MISTAKES & x) >> 0)
#define GET_EFFORT_VALUE(x)        ((MASK_EFFORT & x) >> 2)
#define GET_OVERFEED_VALUE(x)      ((MASK_OVERFEED & x) >> 4)

// Setters
#define SET_CARE_MISTAKES_VALUE(variable, value) \
    variable &= ~MASK_CARE_MISTAKES;             \
    variable |= ((value & 0b00000011) << 0)
#define SET_EFFORT_VALUE(variable, value) \
    variable &= ~MASK_EFFORT;             \
    variable |= ((value & 0b00000011) << 2)
#define SET_OVERFEED_VALUE(variable, value) \
    variable &= ~MASK_OVERFEED;             \
    variable |= ((value & 0b00000011) << 4)

#define MAX_POSSIBLE_EVOLUTIONS 5

typedef struct {
    uint8_t uiProgressionNeeded;
    uint8_t uiWinCount;
    uint16_t uiWaitTime;
} evolution_requirement_t;

typedef struct digimon_t {
    char szName[50];
    uint8_t uiPower;
    uint16_t uiTimeSleep;
    uint8_t uiAttribute;

    uint8_t uiCountPossibleEvolutions;
    evolution_requirement_t* vstEvolutionRequirements[MAX_POSSIBLE_EVOLUTIONS];
    struct digimon_t* vstPossibleEvolutions[MAX_POSSIBLE_EVOLUTIONS];
} digimon_t;

typedef struct {
    digimon_t* pstCurrentDigimon;
    uint8_t uiStats;
    uint8_t uiEvolutionProgression;
    uint8_t uiWeight;
    uint8_t uiWinCount;

    uint16_t uiTimeSinceLastMeal;
    uint16_t uiTimeSinceLastTraining;
} playing_digimon_t;

uint8_t DIGI_evolveDigimon(const playing_digimon_t* pstVerifyingDigimon);

uint8_t DIGI_feedDigimon(playing_digimon_t* pstFedDigimon, int16_t uiAmount);

uint8_t DIGI_stregthenDigimon(playing_digimon_t* pstTreatedDigimon,
                              int16_t uiAmount);

uint8_t DIGI_healDigimon(playing_digimon_t* pstHealedDigimon, uint8_t uiType);

#endif  // DIGIMON_H