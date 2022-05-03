#include "digimon.h"

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
    return DIGI_NO_EVOLUTION;
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
            SET_SICK_VALUE(stPlayingDigimon.uiStats, 1);
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
        SET_SICK_VALUE(stPlayingDigimon.uiStats, 1);
        printf("[DIGILIB] Digmon got sick from obesity\n");
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_healDigimon(uint8_t uiType) {
    printf("[DIGILIB] Healing %s, type %d\n",
           stPlayingDigimon.pstCurrentDigimon->szName, uiType);
    if (uiType == MASK_SICK) {
        uint8_t uiSickStatus = GET_SICK_VALUE(stPlayingDigimon.uiStats);
        if (!uiSickStatus) {
            printf("[DIGILIB] Not sick\n");
            return DIGI_RET_ERROR;
        }

        printf("[DIGILIB] Sickness healed!\n");
        SET_SICK_VALUE(stPlayingDigimon.uiStats, 0);
    } else if (uiType == MASK_INJURIED) {
        uint8_t uiSickStatus = GET_INJURIED_VALUE(stPlayingDigimon.uiStats);
        if (!uiSickStatus) {
            printf("[DIGILIB] Not injuried\n");
            return DIGI_RET_ERROR;
        }

        printf("[DIGILIB] Injury healed!\n");
        SET_INJURIED_VALUE(stPlayingDigimon.uiStats, 0);
    }

    return DIGI_RET_OK;
}
