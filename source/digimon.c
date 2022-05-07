#include "digimon.h"

#include "digihardware.h"
#include "enums.h"

#include <stdio.h>

#define NOT_IN_RANGE(value, min, max) (value < min || value > max)

extern playing_digimon_t stPlayingDigimon;

static uint8_t checkIfValidParameter(const uint16_t uiRange,
                                     const uint8_t uiCheckValue) {
    uint8_t uiMinValue = GET_MIN_VALUE(uiRange),
            uiMaxValue = GET_MAX_VALUE(uiRange);

    return NOT_IN_RANGE(uiCheckValue, uiMinValue, uiMaxValue);
}

uint8_t DIGI_evolveDigimon() {
    int i;
    digimon_t* pstDigimonRaised = stPlayingDigimon.pstCurrentDigimon;

    for (i = 0; i < pstDigimonRaised->uiCountPossibleEvolutions; i++) {
        evolution_requirement_t* pstCurrentEvolution =
            pstDigimonRaised->vstEvolutionRequirements[i];

        printf("[DIGILIB] Testing evolution to %s\n",
               stPlayingDigimon.pstCurrentDigimon->vstPossibleEvolutions[i]
                   ->szName);

        if (NEEDS_CARE_MISTAKES(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for care mistakes (%x)\n",
                   pstCurrentEvolution->uiCareMistakesCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiCareMistakesCount,
                                      stPlayingDigimon.uiCareMistakesCount)) {
                printf("[DIGILIB] Exceeded care mistakes\n");
                continue;
            }
        }
        if (NEEDS_TRAINING(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for training (%x)\n",
                   pstCurrentEvolution->uiTrainingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiTrainingCount,
                                      stPlayingDigimon.uiTrainingCount)) {
                printf("[DIGILIB] Exceeded training\n");
                continue;
            }
        }
        if (NEEDS_OVERFEED(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for overfeeding (%x)\n",
                   pstCurrentEvolution->uiOverfeedingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiOverfeedingCount,
                                      stPlayingDigimon.uiOverfeedingCount)) {
                printf("[DIGILIB] Exceeded overfeeding\n");
                continue;
            }
        }
        if (NEEDS_SLEEP_DISTURBANCE(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for sleep disturbance (%x)\n",
                   pstCurrentEvolution->uiSleepDisturbanceCount);

            if (checkIfValidParameter(
                    pstCurrentEvolution->uiSleepDisturbanceCount,
                    stPlayingDigimon.uiSleepDisturbanceCount)) {
                printf("[DIGILIB] Exceeded sleep disturbance\n");
                continue;
            }
        }
        if (NEEDS_WIN_COUNT(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need victories (%x)\n",
                   pstCurrentEvolution->uiWinCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiWinCount,
                                      stPlayingDigimon.uiWinCount)) {
                printf("[DIGILIB] Exceeded victories\n");
                continue;
            }
        }

        stPlayingDigimon.pstCurrentDigimon =
            stPlayingDigimon.pstCurrentDigimon->vstPossibleEvolutions[i];
        stPlayingDigimon.uiTimeToEvolve = 0;
        stPlayingDigimon.uiCareMistakesCount = 0;
        stPlayingDigimon.uiTrainingCount = 0;
        stPlayingDigimon.uiOverfeedingCount = 0;
        stPlayingDigimon.uiSleepDisturbanceCount = 0;

        return DIGI_RET_OK;
    }

    printf("[DIGILIB] No valid evolution");
    stPlayingDigimon.uiStats |= MASK_DYING_STAGE;
    return DIGI_NO_EVOLUTION;
}

uint8_t DIGI_shouldEvolve() {
    return stPlayingDigimon.uiTimeToEvolve >=
                       stPlayingDigimon.pstCurrentDigimon
                           ->uiNeededTimeEvolution &&
                   (stPlayingDigimon.uiStats & MASK_DYING_STAGE) == 0
               ? DIGI_RET_OK
               : DIGI_RET_ERROR;
}

uint8_t DIGI_feedDigimon(int16_t uiAmount) {
    int8_t iCurrentHungerAmount =
        GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength);

    printf("[DIGILIB] Feeding %s, ",
           stPlayingDigimon.pstCurrentDigimon->szName);

    // Deixa o digimon mais cheio (o SET já garante que não vai ser um valor maior que o permitido)
    iCurrentHungerAmount += uiAmount;
    if (iCurrentHungerAmount <= 0) {
        stPlayingDigimon.uiHungerStrength &= ~MASK_HUNGER;
        return DIGI_RET_HUNGRY;
    } else if (iCurrentHungerAmount <= 4) {
        SET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength,
                         iCurrentHungerAmount);
    }

    printf("new amout %d (real value %d)\n", iCurrentHungerAmount,
           GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength));

    // Aumenta o peso, se estiver obeso, deixa doente.
    if (uiAmount > 0) {
        stPlayingDigimon.uiWeight++;

        if (stPlayingDigimon.uiWeight >= 99) {
            stPlayingDigimon.uiWeight = 99;
            stPlayingDigimon.uiStats |= MASK_SICK;
            printf("[DIGILIB] Digimon got sick from being obese\n");
        }
    }

    // Se foi dado comida apesar dele estar cheio, marca como overfeed.
    if (iCurrentHungerAmount > 3) {
        stPlayingDigimon.uiOverfeedingCount++;
        printf("[DIGILIB] %s got overfed\n",
               stPlayingDigimon.pstCurrentDigimon->szName);
        return DIGI_RET_OVERFEED;
    }

    // Se não foi alimentado ok
    return DIGI_RET_OK;
}

uint8_t DIGI_stregthenDigimon(int16_t uiAmount) {
    printf("[DIGILIB] Strengthening %s by %d\n",
           stPlayingDigimon.pstCurrentDigimon->szName, uiAmount);

    int8_t iCurrentStrength =
        GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength);

    // Aumenta a força do digimon (o set já garante que não vai ser um valor maior que o permitido)
    iCurrentStrength += uiAmount;
    if (iCurrentStrength <= 0) {
        printf("[DIGILIB] Digmon has no strength left\n");
        stPlayingDigimon.uiHungerStrength &= ~MASK_STRENGTH;
        return DIGI_RET_WEAK;
    } else if (iCurrentStrength <= 4) {
        SET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength, iCurrentStrength);
    }

    // Aumenta o peso, se estiver obeso, deixa doente.
    stPlayingDigimon.uiWeight++;
    if (stPlayingDigimon.uiWeight >= 99) {
        stPlayingDigimon.uiWeight = 99;
        stPlayingDigimon.uiStats |= MASK_SICK;
        printf("[DIGILIB] Digmon got sick from obesity\n");
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_healDigimon(uint8_t uiType) {
    printf("[DIGILIB] Healing %s, type %d\n",
           stPlayingDigimon.pstCurrentDigimon->szName, uiType);
    if (uiType == MASK_SICK) {
        if ((stPlayingDigimon.uiStats & MASK_SICK) == 0) {
            printf("[DIGILIB] Not sick\n");
            return DIGI_RET_ERROR;
        }

        printf("[DIGILIB] Sickness healed!\n");
        stPlayingDigimon.uiStats &= ~MASK_SICK;
    } else if (uiType == MASK_INJURIED) {
        if ((stPlayingDigimon.uiStats & MASK_INJURIED) == 0) {
            printf("[DIGILIB] Not injuried\n");
            return DIGI_RET_ERROR;
        }

        printf("[DIGILIB] Injury healed!\n");
        stPlayingDigimon.uiStats &= ~MASK_INJURIED;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_putSleep(uint8_t uiSleepMode) {
    printf("[DIGILIB] Putting %s to sleep -> %d\n",
           stPlayingDigimon.pstCurrentDigimon->szName, uiSleepMode);

    uiSleepMode &= 1;

    if ((stPlayingDigimon.uiStats & MASK_SLEEPING) != 0 && uiSleepMode) {
        printf("[DIGILIB] It is a sleep disturbance\n");
        stPlayingDigimon.uiSleepDisturbanceCount++;
    }

    stPlayingDigimon.uiStats &= ~MASK_SLEEPING;
    stPlayingDigimon.uiStats |= uiSleepMode << 6;
}

uint8_t DIGI_shouldSleep() {
    const digimon_t* pstCurrentDigimon = stPlayingDigimon.pstCurrentDigimon;
    const uint16_t uiCurrentTime = DIGIHW_timeMinutes();

    if (pstCurrentDigimon->uiStage <= DIGI_STAGE_BABY_1)
        return DIGI_RET_ERROR;
    else if ((stPlayingDigimon.uiStats & MASK_SLEEPING) != 0)
        return DIGI_RET_ERROR;

    if (uiCurrentTime > pstCurrentDigimon->uiTimeSleep)
        return DIGI_RET_OK;
    if (uiCurrentTime > pstCurrentDigimon->uiTimeWakeUp)
        return DIGI_RET_ERROR;
    return DIGI_RET_OK;
}

uint8_t DIGI_setCalled() {
    stPlayingDigimon.uiStats &= ~MASK_CALLED;

    if (GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength) == 0)
        stPlayingDigimon.uiStats |= MASK_CALLED;
    else if (GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength) == 0)
        stPlayingDigimon.uiStats |= MASK_CALLED;
    else if (DIGI_shouldSleep() == DIGI_RET_OK)
        stPlayingDigimon.uiStats |= MASK_CALLED;

    return stPlayingDigimon.uiStats & MASK_CALLED ? DIGI_RET_OK
                                                  : DIGI_RET_ERROR;
}

void DIGI_addCareMistakes() {
    if (GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength) == 0)
        stPlayingDigimon.uiCareMistakesCount++;
    else if (GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength) == 0)
        stPlayingDigimon.uiCareMistakesCount++;
    else if (DIGI_shouldSleep() == DIGI_RET_OK)
        stPlayingDigimon.uiCareMistakesCount++;
}
