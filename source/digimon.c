#include "digimon.h"

#include "enums.h"

#include <stdio.h>

#define NOT_IN_RANGE(value, min, max) (value < min || value > max)

static uint8_t checkIfValidParameter(const uint16_t uiRange,
                                     const uint8_t uiCheckValue) {
    uint8_t uiMinValue = GET_MIN_VALUE(uiRange),
            uiMaxValue = GET_MAX_VALUE(uiRange);

    return NOT_IN_RANGE(uiCheckValue, uiMinValue, uiMaxValue);
}

uint8_t DIGI_evolveDigimon(playing_digimon_t* pstVerifyingDigimon) {
    int i;
    digimon_t* pstDigimonRaised = pstVerifyingDigimon->pstCurrentDigimon;

    for (i = 0; i < pstDigimonRaised->uiCountPossibleEvolutions; i++) {
        evolution_requirement_t* pstCurrentEvolution =
            pstDigimonRaised->vstEvolutionRequirements[i];

        printf("[DIGILIB] Testing evolution to %s\n",
               pstVerifyingDigimon->pstCurrentDigimon->vstPossibleEvolutions[i]
                   ->szName);

        if (NEEDS_CARE_MISTAKES(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for care mistakes (%x)\n",
                   pstCurrentEvolution->uiCareMistakesCount);

            if (checkIfValidParameter(
                    pstCurrentEvolution->uiCareMistakesCount,
                    pstVerifyingDigimon->uiCareMistakesCount)) {
                printf("[DIGILIB] Exceeded care mistakes\n");
                continue;
            }
        }
        if (NEEDS_TRAINING(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for training (%x)\n",
                   pstCurrentEvolution->uiTrainingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiTrainingCount,
                                      pstVerifyingDigimon->uiTrainingCount)) {
                printf("[DIGILIB] Exceeded training\n");
                continue;
            }
        }
        if (NEEDS_OVERFEED(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for overfeeding (%x)\n",
                   pstCurrentEvolution->uiOverfeedingCount);

            if (checkIfValidParameter(
                    pstCurrentEvolution->uiOverfeedingCount,
                    pstVerifyingDigimon->uiOverfeedingCount)) {
                printf("[DIGILIB] Exceeded overfeeding\n");
                continue;
            }
        }
        if (NEEDS_SLEEP_DISTURBANCE(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need for sleep disturbance (%x)\n",
                   pstCurrentEvolution->uiSleepDisturbanceCount);

            if (checkIfValidParameter(
                    pstCurrentEvolution->uiSleepDisturbanceCount,
                    pstVerifyingDigimon->uiSleepDisturbanceCount)) {
                printf("[DIGILIB] Exceeded sleep disturbance\n");
                continue;
            }
        }
        if (NEEDS_WIN_COUNT(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] It has need victories (%x)\n",
                   pstCurrentEvolution->uiWinCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiWinCount,
                                      pstVerifyingDigimon->uiWinCount)) {
                printf("[DIGILIB] Exceeded victories\n");
                continue;
            }
        }

        pstVerifyingDigimon->pstCurrentDigimon =
            pstVerifyingDigimon->pstCurrentDigimon->vstPossibleEvolutions[i];
        pstVerifyingDigimon->uiTimeToEvolve = 0;
        pstVerifyingDigimon->uiCareMistakesCount = 0;
        pstVerifyingDigimon->uiTrainingCount = 0;
        pstVerifyingDigimon->uiOverfeedingCount = 0;
        pstVerifyingDigimon->uiSleepDisturbanceCount = 0;

        return DIGI_RET_OK;
    }

    printf("[DIGILIB] No valid evolution");
    return DIGI_NO_EVOLUTION;
}

uint8_t DIGI_feedDigimon(playing_digimon_t* pstFedDigimon, int16_t uiAmount) {
    int8_t iCurrentHungerAmount =
        GET_HUNGER_VALUE(pstFedDigimon->uiHungerStrength);

    printf("[DIGILIB] Feeding %s, ", pstFedDigimon->pstCurrentDigimon->szName);

    // Deixa o digimon mais cheio (o SET já garante que não vai ser um valor maior que o permitido)
    iCurrentHungerAmount += uiAmount;
    if (iCurrentHungerAmount <= 0) {
        pstFedDigimon->uiHungerStrength &= ~MASK_HUNGER;
        return DIGI_RET_HUNGRY;
    } else if (iCurrentHungerAmount <= 4) {
        SET_HUNGER_VALUE(pstFedDigimon->uiHungerStrength, iCurrentHungerAmount);
    }

    printf("new amout %d (real value %d)\n", iCurrentHungerAmount,
           GET_HUNGER_VALUE(pstFedDigimon->uiHungerStrength));

    // Aumenta o peso, se estiver obeso, deixa doente.
    if (uiAmount > 0) {
        pstFedDigimon->uiWeight++;

        if (pstFedDigimon->uiWeight >= 99) {
            pstFedDigimon->uiWeight = 99;
            SET_SICK_VALUE(pstFedDigimon->uiStats, 1);
            printf("[DIGILIB] Digimon got sick from being obese\n");
        }
    }

    // Se foi dado comida apesar dele estar cheio, marca como overfeed.
    if (iCurrentHungerAmount > 3) {
        pstFedDigimon->uiOverfeedingCount++;
        printf("[DIGILIB] %s got overfed\n",
               pstFedDigimon->pstCurrentDigimon->szName);
        return DIGI_RET_OVERFEED;
    }

    // Se não foi alimentado ok
    return DIGI_RET_OK;
}

uint8_t DIGI_stregthenDigimon(playing_digimon_t* pstTreatedDigimon,
                              int16_t uiAmount) {
    printf("[DIGILIB] Strengthening %s by %d\n",
           pstTreatedDigimon->pstCurrentDigimon->szName, uiAmount);

    int8_t iCurrentStrength =
        GET_STRENGTH_VALUE(pstTreatedDigimon->uiHungerStrength);

    // Aumenta a força do digimon (o set já garante que não vai ser um valor maior que o permitido)
    iCurrentStrength += uiAmount;
    if (iCurrentStrength <= 0) {
        printf("[DIGILIB] Digmon has no strength left\n");
        pstTreatedDigimon->uiHungerStrength &= ~MASK_STRENGTH;
        return DIGI_RET_WEAK;
    } else if (iCurrentStrength <= 4) {
        SET_STRENGTH_VALUE(pstTreatedDigimon->uiHungerStrength,
                           iCurrentStrength);
    }

    // Aumenta o peso, se estiver obeso, deixa doente.
    pstTreatedDigimon->uiWeight++;
    if (pstTreatedDigimon->uiWeight >= 99) {
        pstTreatedDigimon->uiWeight = 99;
        SET_SICK_VALUE(pstTreatedDigimon->uiStats, 1);
        printf("[DIGILIB] Digmon got sick from obesity\n");
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_healDigimon(playing_digimon_t* pstHealedDigimon, uint8_t uiType) {
    printf("[DIGILIB] Healing %s, type %d\n",
           pstHealedDigimon->pstCurrentDigimon->szName, uiType);
    if (uiType == MASK_SICK) {
        uint8_t uiSickStatus = GET_SICK_VALUE(pstHealedDigimon->uiStats);
        if (!uiSickStatus) {
            printf("[DIGILIB] Not sick\n");
            return DIGI_RET_ERROR;
        }

        printf("[DIGILIB] Sickness healed!\n");
        SET_SICK_VALUE(pstHealedDigimon->uiStats, 0);
    } else if (uiType == MASK_INJURIED) {
        uint8_t uiSickStatus = GET_INJURIED_VALUE(pstHealedDigimon->uiStats);
        if (!uiSickStatus) {
            printf("[DIGILIB] Not injuried\n");
            return DIGI_RET_ERROR;
        }

        printf("[DIGILIB] Injury healed!\n");
        SET_INJURIED_VALUE(pstHealedDigimon->uiStats, 0);
    }

    return DIGI_RET_OK;
}
