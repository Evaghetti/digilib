#include "digimon.h"

#include "enums.h"

#include <stdio.h>

uint8_t DIGI_evolveDigimon(const playing_digimon_t* pstVerifyingDigimon) {
    int i;
    digimon_t* pstDigimonRaised = pstVerifyingDigimon->pstCurrentDigimon;

    for (i = 0; i < pstDigimonRaised->uiCountPossibleEvolutions; i++) {
        evolution_requirement_t* pstCurrentEvolution =
            pstDigimonRaised->vstEvolutionRequirements[i];

        printf("[DIGILIB] Verifying evolution to %s\n",
               pstDigimonRaised->vstPossibleEvolutions[i]->szName);
        if (pstCurrentEvolution->uiProgressionNeeded & MASK_NEEDS_OVERFEED) {
            printf("[DIGILIB] Verifying for amount of overfeeding\n");
            uint8_t uiCountOverfeed =
                GET_OVERFEED_VALUE(pstVerifyingDigimon->uiEvolutionProgression);
            uint8_t uiNeededOverfeed =
                GET_OVERFEED_VALUE(pstCurrentEvolution->uiProgressionNeeded);

            if (uiCountOverfeed < uiNeededOverfeed) {
                printf("[DIGILIB] Not enough overfeeding (%d < %d)\n",
                       uiCountOverfeed, uiNeededOverfeed);
                continue;
            }
        }

        if (pstCurrentEvolution->uiProgressionNeeded & MASK_NEEDS_WIN_COUNT) {
            printf("[DIGILIB] Verifying for amount of wins\n");
            if (pstVerifyingDigimon->uiWinCount <
                pstCurrentEvolution->uiWinCount) {
                printf("[DIGILIB] Not enough wins (%d < %d)\n",
                       pstVerifyingDigimon->uiWinCount,
                       pstCurrentEvolution->uiWinCount);
            }
            continue;
        }

        if (GET_CARE_MISTAKES_VALUE(
                pstVerifyingDigimon->uiEvolutionProgression) >
            GET_CARE_MISTAKES_VALUE(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] Exceeded care mistakes\n");
            continue;
        }
        if (GET_EFFORT_VALUE(pstVerifyingDigimon->uiEvolutionProgression) >
            GET_EFFORT_VALUE(pstCurrentEvolution->uiProgressionNeeded)) {
            printf("[DIGILIB] Exceeded effort\n");
            continue;
        }

        return i;
    }

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
        iCurrentHungerAmount =
            GET_OVERFEED_VALUE(pstFedDigimon->uiEvolutionProgression);
        SET_OVERFEED_VALUE(pstFedDigimon->uiEvolutionProgression,
                           ++iCurrentHungerAmount);
        printf("[DIGILIB] %s got overfed\n",
               pstFedDigimon->pstCurrentDigimon->szName);
        return DIGI_RET_OVERFEED;
    }

    // Se não foi alimentado ok
    return DIGI_RET_OK;
}

uint8_t DIGI_stregthenDigimon(playing_digimon_t* pstTreatedDigimon,
                              int16_t uiAmount) {
    int8_t iCurrentStrength =
        GET_STRENGTH_VALUE(pstTreatedDigimon->uiHungerStrength);

    // Aumenta a força do digimon (o set já garante que não vai ser um valor maior que o permitido)
    iCurrentStrength += uiAmount;
    if (iCurrentStrength <= 0) {
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
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_healDigimon(playing_digimon_t* pstHealedDigimon, uint8_t uiType) {
    if (uiType == MASK_SICK) {
        uint8_t uiSickStatus = GET_SICK_VALUE(pstHealedDigimon->uiStats);
        if (!uiSickStatus)
            return DIGI_RET_ERROR;

        SET_SICK_VALUE(pstHealedDigimon->uiStats, 0);
    } else if (uiType == MASK_INJURIED) {
        uint8_t uiSickStatus = GET_INJURIED_VALUE(pstHealedDigimon->uiStats);
        if (!uiSickStatus)
            return DIGI_RET_ERROR;

        SET_INJURIED_VALUE(pstHealedDigimon->uiStats, 0);
    }

    return DIGI_RET_OK;
}
