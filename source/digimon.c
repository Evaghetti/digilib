#include "digimon.h"

#include "digihardware.h"
#include "digiworld.h"
#include "enums.h"
#include "logging.h"

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

        LOG("Testing evolution to %s",
            vstPossibleDigimon[pstCurrentEvolution->uiIndexEvolution].szName);

        if (NEEDS_CARE_MISTAKES(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for care mistakes (%x)",
                pstCurrentEvolution->uiCareMistakesCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiCareMistakesCount,
                                      stPlayingDigimon.uiCareMistakesCount)) {
                LOG("Exceeded care mistakes");
                continue;
            }
        }
        if (NEEDS_TRAINING(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for training (%x)",
                pstCurrentEvolution->uiTrainingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiTrainingCount,
                                      stPlayingDigimon.uiTrainingCount)) {
                LOG("Exceeded training");
                continue;
            }
        }
        if (NEEDS_OVERFEED(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for overfeeding (%x)",
                pstCurrentEvolution->uiOverfeedingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiOverfeedingCount,
                                      stPlayingDigimon.uiOverfeedingCount)) {
                LOG("Exceeded overfeeding");
                continue;
            }
        }
        if (NEEDS_SLEEP_DISTURBANCE(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for sleep disturbance (%x)",
                pstCurrentEvolution->uiSleepDisturbanceCount);

            if (checkIfValidParameter(
                    pstCurrentEvolution->uiSleepDisturbanceCount,
                    stPlayingDigimon.uiSleepDisturbanceCount)) {
                LOG("Exceeded sleep disturbance");
                continue;
            }
        }
        if (NEEDS_WIN_COUNT(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need victories (%x)", pstCurrentEvolution->uiWinCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiWinCount,
                                      stPlayingDigimon.uiWinCount)) {
                LOG("Exceeded victories");
                continue;
            }
        }

        stPlayingDigimon.uiIndexCurrentDigimon =
            pstCurrentEvolution->uiIndexEvolution;
        stPlayingDigimon.pstCurrentDigimon =
            &vstPossibleDigimon[stPlayingDigimon.uiIndexCurrentDigimon];
        stPlayingDigimon.uiTimeToEvolve = 0;
        stPlayingDigimon.uiCareMistakesCount = 0;
        stPlayingDigimon.uiTrainingCount = 0;
        stPlayingDigimon.uiOverfeedingCount = 0;
        stPlayingDigimon.uiSleepDisturbanceCount = 0;

        return DIGI_RET_OK;
    }

    LOG("No valid evolution");
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

    LOG("Feeding %s, amount of ", stPlayingDigimon.pstCurrentDigimon->szName,
        uiAmount);

    // Deixa o digimon mais cheio (o SET já garante que não vai ser um valor maior que o permitido)
    iCurrentHungerAmount += uiAmount;
    if (iCurrentHungerAmount <= 0) {
        stPlayingDigimon.uiHungerStrength &= ~MASK_HUNGER;
        return DIGI_RET_HUNGRY;
    } else if (iCurrentHungerAmount <= 4) {
        SET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength,
                         iCurrentHungerAmount);
    }
    // Aumenta o peso, se estiver obeso, deixa doente.
    if (uiAmount > 0) {
        stPlayingDigimon.uiWeight++;

        if (stPlayingDigimon.uiWeight >= 99) {
            stPlayingDigimon.uiWeight = 99;
            stPlayingDigimon.uiStats |= MASK_SICK;
            LOG("Digimon got sick from being obese");
        }
    }

    // Se foi dado comida apesar dele estar cheio, marca como overfeed.
    if (iCurrentHungerAmount > 4) {
        stPlayingDigimon.uiOverfeedingCount++;
        LOG("%s got overfed", stPlayingDigimon.pstCurrentDigimon->szName);
        return DIGI_RET_OVERFEED;
    }

    // Se não foi alimentado ok
    return DIGI_RET_OK;
}

uint8_t DIGI_stregthenDigimon(int16_t uiAmount, int8_t iWeightChange) {
    LOG("Strengthening %s by %d", stPlayingDigimon.pstCurrentDigimon->szName,
        uiAmount);

    int8_t iCurrentStrength =
        GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength);

    // Aumenta a força do digimon (o set já garante que não vai ser um valor maior que o permitido)
    iCurrentStrength += uiAmount;
    if (iCurrentStrength <= 0) {
        LOG("Digmon has no strength left");
        stPlayingDigimon.uiHungerStrength &= ~MASK_STRENGTH;
        return DIGI_RET_WEAK;
    } else if (iCurrentStrength <= 4) {
        SET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength, iCurrentStrength);
    }

    // Aumenta o peso, se estiver obeso, deixa doente.
    stPlayingDigimon.uiWeight += iWeightChange;
    if (stPlayingDigimon.uiWeight < 10) {
        stPlayingDigimon.uiWeight = 10;
    } else if (stPlayingDigimon.uiWeight >= 99) {
        stPlayingDigimon.uiWeight = 99;
        stPlayingDigimon.uiStats |= MASK_SICK;
        LOG("Digmon got sick from obesity");
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_healDigimon(uint8_t uiType) {
    LOG("Healing %s, type %d", stPlayingDigimon.pstCurrentDigimon->szName,
        uiType);
    if (uiType == MASK_SICK) {
        if ((stPlayingDigimon.uiStats & MASK_SICK) == 0) {
            LOG("Not sick");
            return DIGI_RET_ERROR;
        }

        LOG("Sickness healed!");
        stPlayingDigimon.uiStats &= ~MASK_SICK;
    } else if (uiType == MASK_INJURIED) {
        if ((stPlayingDigimon.uiStats & MASK_INJURIED) == 0) {
            LOG("Not injuried");
            return DIGI_RET_ERROR;
        }

        LOG("Injury healed!");
        stPlayingDigimon.uiStats &= ~MASK_INJURIED;
    }

    return DIGI_RET_OK;
}

uint8_t DIGI_putSleep(uint8_t uiSleepMode) {
    LOG("Putting %s to sleep -> %d", stPlayingDigimon.pstCurrentDigimon->szName,
        uiSleepMode);

    uiSleepMode &= 1;

    if ((stPlayingDigimon.uiStats & MASK_SLEEPING) != 0 && uiSleepMode) {
        LOG("It is a sleep disturbance");
        stPlayingDigimon.uiSleepDisturbanceCount++;
    }

    stPlayingDigimon.uiStats &= ~MASK_SLEEPING;
    stPlayingDigimon.uiStats |= uiSleepMode << 6;
    return DIGI_RET_OK;
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

uint8_t DIGI_shouldWakeUp() {
    const digimon_t* pstCurrentDigimon = stPlayingDigimon.pstCurrentDigimon;
    const uint16_t uiCurrentTime = DIGIHW_timeMinutes();

    if (uiCurrentTime >= pstCurrentDigimon->uiTimeWakeUp &&
        uiCurrentTime < pstCurrentDigimon->uiTimeSleep &&
        (stPlayingDigimon.uiStats & MASK_SLEEPING) != 0)
        return DIGI_RET_OK;
    return DIGI_RET_ERROR;
}

uint8_t DIGI_setCalled() {
    stPlayingDigimon.uiStats &= ~MASK_CALLED;

    if (GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength) == 0) {
        stPlayingDigimon.uiStats |= MASK_CALLED;
        LOG("Will be calling because of hunger");
    } else if (GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength) == 0) {
        stPlayingDigimon.uiStats |= MASK_CALLED;
        LOG("Will be calling because of strength");
    } else if (DIGI_shouldSleep() == DIGI_RET_OK) {
        stPlayingDigimon.uiStats |= MASK_CALLED;
        LOG("Will be calling because of sleep");
    }

    return stPlayingDigimon.uiStats & MASK_CALLED ? DIGI_RET_OK
                                                  : DIGI_RET_ERROR;
}

void DIGI_addCareMistakes() {
    LOG("HungerStrength %04x, shouldSleep() == %d",
        stPlayingDigimon.uiHungerStrength, DIGI_shouldSleep());
    if (GET_HUNGER_VALUE(stPlayingDigimon.uiHungerStrength) == 0)
        stPlayingDigimon.uiCareMistakesCount++;
    else if (GET_STRENGTH_VALUE(stPlayingDigimon.uiHungerStrength) == 0)
        stPlayingDigimon.uiCareMistakesCount++;
    else if (DIGI_shouldSleep() == DIGI_RET_OK)
        stPlayingDigimon.uiCareMistakesCount++;
}
