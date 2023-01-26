#include "digimon.h"

#include "digihal.h"
#include "digiworld.h"
#include "enums.h"

#include <stdio.h>

#define NOT_IN_RANGE(value, min, max) (value < min || value > max)

#define NOT_COUNTING_FOR_CARE_MISTAKE 0

extern uint32_t guiCurrentTime;

static uint8_t checkIfValidParameter(const uint16_t uiRange,
                                     const uint8_t uiCheckValue) {
    uint8_t uiMinValue = GET_MIN_VALUE(uiRange),
            uiMaxValue = GET_MAX_VALUE(uiRange);

    return NOT_IN_RANGE(uiCheckValue, uiMinValue, uiMaxValue);
}

uint8_t DIGI_evolveDigimon(playing_digimon_t* pstPlayingDigimon) {
    int i;
    digimon_t* pstDigimonRaised = pstPlayingDigimon->pstCurrentDigimon;

    for (i = 0; i < pstDigimonRaised->uiCountPossibleEvolutions; i++) {
        evolution_requirement_t* pstCurrentEvolution =
            pstDigimonRaised->vstEvolutionRequirements[i];

        LOG("Testing evolution to %s",
            vstPossibleDigimon[pstCurrentEvolution->uiIndexEvolution].szName);

        if (NEEDS_CARE_MISTAKES(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for care mistakes (%x)",
                pstCurrentEvolution->uiCareMistakesCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiCareMistakesCount,
                                      pstPlayingDigimon->uiCareMistakesCount)) {
                LOG("Exceeded care mistakes");
                continue;
            }
        }
        if (NEEDS_TRAINING(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for training (%x)",
                pstCurrentEvolution->uiTrainingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiTrainingCount,
                                      pstPlayingDigimon->uiTrainingCount)) {
                LOG("Exceeded training");
                continue;
            }
        }
        if (NEEDS_OVERFEED(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for overfeeding (%x)",
                pstCurrentEvolution->uiOverfeedingCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiOverfeedingCount,
                                      pstPlayingDigimon->uiOverfeedingCount)) {
                LOG("Exceeded overfeeding");
                continue;
            }
        }
        if (NEEDS_SLEEP_DISTURBANCE(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need for sleep disturbance (%x)",
                pstCurrentEvolution->uiSleepDisturbanceCount);

            if (checkIfValidParameter(
                    pstCurrentEvolution->uiSleepDisturbanceCount,
                    pstPlayingDigimon->uiSleepDisturbanceCount)) {
                LOG("Exceeded sleep disturbance");
                continue;
            }
        }
        if (NEEDS_WIN_COUNT(pstCurrentEvolution->uiProgressionNeeded)) {
            LOG("It has need victories (%x)", pstCurrentEvolution->uiWinCount);

            if (checkIfValidParameter(pstCurrentEvolution->uiWinCount,
                                      pstPlayingDigimon->uiWinCount)) {
                LOG("Exceeded victories");
                continue;
            }
        }

        pstPlayingDigimon->uiIndexCurrentDigimon =
            pstCurrentEvolution->uiIndexEvolution;
        pstPlayingDigimon->pstCurrentDigimon =
            &vstPossibleDigimon[pstPlayingDigimon->uiIndexCurrentDigimon];
        pstPlayingDigimon->uiTimeToEvolve = 0;
        pstPlayingDigimon->uiCareMistakesCount = 0;
        pstPlayingDigimon->uiTrainingCount = 0;
        pstPlayingDigimon->uiOverfeedingCount = 0;
        pstPlayingDigimon->uiSleepDisturbanceCount = 0;
        pstPlayingDigimon->uiSickCount = 0;
        pstPlayingDigimon->uiInjuredCount = 0;

        return DIGI_RET_OK;
    }

    LOG("No valid evolution");
    pstPlayingDigimon->uiStats |= MASK_DYING_STAGE;
    return DIGI_NO_EVOLUTION;
}

uint8_t DIGI_shouldEvolve(const playing_digimon_t* pstPlayingDigimon) {
    return pstPlayingDigimon->uiTimeToEvolve >=
                       pstPlayingDigimon->pstCurrentDigimon
                           ->uiNeededTimeEvolution &&
                   (pstPlayingDigimon->uiStats & MASK_DYING_STAGE) == 0
               ? DIGI_RET_OK
               : DIGI_RET_ERROR;
}

uint8_t DIGI_feedDigimon(playing_digimon_t* pstPlayingDigimon,
                         int16_t uiAmount) {
    int8_t iCurrentHungerAmount =
        GET_HUNGER_VALUE(pstPlayingDigimon->uiHungerStrength);

    LOG("Feeding %s, amount of ", pstPlayingDigimon->pstCurrentDigimon->szName,
        uiAmount);

    // Deixa o digimon mais cheio (o SET já garante que não vai ser um valor maior que o permitido)
    iCurrentHungerAmount += uiAmount;
    if (iCurrentHungerAmount <= 0) {
        pstPlayingDigimon->uiHungerStrength &= ~MASK_HUNGER;
        return DIGI_RET_HUNGRY;
    } else if (iCurrentHungerAmount <= 4) {
        if (GET_HUNGER_VALUE(pstPlayingDigimon->uiHungerStrength) >
            iCurrentHungerAmount)
            pstPlayingDigimon->uiTimedFlags |= DIGI_TIMEDFLG_CAN_OVERFEED;

        SET_HUNGER_VALUE(pstPlayingDigimon->uiHungerStrength,
                         iCurrentHungerAmount);
    }
    // Aumenta o peso, se estiver obeso, deixa doente.
    if (uiAmount > 0) {
        pstPlayingDigimon->uiWeight++;

        if (pstPlayingDigimon->uiWeight >= 99) {
            pstPlayingDigimon->uiWeight = 99;
            pstPlayingDigimon->uiStats |= MASK_SICK;
            LOG("Digimon got sick from being obese");
        }
    }

    // Se foi dado comida apesar dele estar cheio, marca como overfeed.
    if (iCurrentHungerAmount > 4) {
        pstPlayingDigimon->uiOverfeedingCount++;
        pstPlayingDigimon->uiTimedFlags &= ~DIGI_TIMEDFLG_CAN_OVERFEED;
        LOG("%s got overfed", pstPlayingDigimon->pstCurrentDigimon->szName);
        return DIGI_RET_OVERFEED;
    }

    // Se não foi alimentado ok
    return DIGI_RET_OK;
}

uint8_t DIGI_stregthenDigimon(playing_digimon_t* pstPlayingDigimon,
                              int16_t uiAmount, int8_t iWeightChange) {
    LOG("Strengthening %s by %d", pstPlayingDigimon->pstCurrentDigimon->szName,
        uiAmount);

    int8_t iCurrentStrength =
        GET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength);

    // Aumenta a força do digimon (o set já garante que não vai ser um valor maior que o permitido)
    iCurrentStrength += uiAmount;
    if (iCurrentStrength <= 0) {
        LOG("Digmon has no strength left");
        pstPlayingDigimon->uiHungerStrength &= ~MASK_STRENGTH;
        return DIGI_RET_WEAK;
    } else if (iCurrentStrength <= 4) {
        SET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength,
                           iCurrentStrength);
    }

    // Aumenta o peso, se estiver obeso, deixa doente.
    pstPlayingDigimon->uiWeight += iWeightChange;
    if (pstPlayingDigimon->uiWeight < 10) {
        pstPlayingDigimon->uiWeight = 10;
    } else if (pstPlayingDigimon->uiWeight >= 99) {
        pstPlayingDigimon->uiWeight = 99;
        pstPlayingDigimon->uiStats |= MASK_SICK;
        LOG("Digmon got sick from obesity");
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

// TODO: Implement effort? 4 training == 1 effort?
uint8_t DIGI_trainDigimon(playing_digimon_t* pstPlayingDigimon,
                          uint8_t uiAmount) {
    LOG("Starting to training digimon, strength %d count %d",
        GET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength),
        pstPlayingDigimon->uiTrainingCount);
    while (uiAmount) {
        pstPlayingDigimon->uiTrainingCount++;
        if (DIGI_stregthenDigimon(pstPlayingDigimon, 1, -1) != DIGI_RET_OK) {
            LOG("Error during training");
            return DIGI_RET_ERROR;
        }

        uiAmount--;
    }
    LOG("Finished training digimon, strength %d count %d",
        GET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength),
        pstPlayingDigimon->uiTrainingCount);

    return DIGI_RET_OK;
}

uint8_t DIGI_healDigimon(playing_digimon_t* pstPlayingDigimon, uint8_t uiType) {
    LOG("Healing %s, type %d", pstPlayingDigimon->pstCurrentDigimon->szName,
        uiType);
    if (uiType == MASK_SICK) {
        if ((pstPlayingDigimon->uiStats & MASK_SICK) == 0) {
            LOG("Not sick");
            return DIGI_RET_ERROR;
        }

        LOG("Sickness healed!");
        pstPlayingDigimon->uiStats &= ~MASK_SICK;
    } else if (uiType == MASK_INJURIED) {
        if ((pstPlayingDigimon->uiStats & MASK_INJURIED) == 0) {
            LOG("Not injuried");
            return DIGI_RET_ERROR;
        }

        LOG("Injury healed!");
        pstPlayingDigimon->uiStats &= ~MASK_INJURIED;
    }

    if ((pstPlayingDigimon->uiStats & (MASK_INJURIED | MASK_SICK)) == 0)
        pstPlayingDigimon->uiTimeSickOrInjured = 0;

    return DIGI_RET_OK;
}

uint8_t DIGI_putSleep(playing_digimon_t* pstPlayingDigimon,
                      uint8_t uiSleepMode) {
    LOG("Putting %s to sleep -> %d",
        pstPlayingDigimon->pstCurrentDigimon->szName, uiSleepMode);

    uiSleepMode &= 1;

    if ((pstPlayingDigimon->uiStats & MASK_SLEEPING) &&
        (pstPlayingDigimon->uiTimedFlags & DIGI_TIMEDFLG_CAN_DISTURB_SLEEP) &&
        !uiSleepMode) {
        LOG("It is a sleep disturbance");

        pstPlayingDigimon->uiSleepDisturbanceCount++;
        pstPlayingDigimon->uiTimedFlags &= ~DIGI_TIMEDFLG_CAN_DISTURB_SLEEP;
    }

    pstPlayingDigimon->uiStats &= ~MASK_SLEEPING;
    pstPlayingDigimon->uiStats |= uiSleepMode << 6;
    return DIGI_RET_OK;
}

uint8_t DIGI_shouldSleep(const playing_digimon_t* pstPlayingDigimon) {
    const digimon_t* pstCurrentDigimon = pstPlayingDigimon->pstCurrentDigimon;
    const uint16_t uiCurrentTime = guiCurrentTime;

    if (!(pstPlayingDigimon->uiTimedFlags & DIGI_TIMEDFLG_CAN_DISTURB_SLEEP))
        return DIGI_RET_ERROR;
    else if (pstCurrentDigimon->uiStage <= DIGI_STAGE_BABY_1)
        return DIGI_RET_ERROR;
    else if ((pstPlayingDigimon->uiStats & MASK_SLEEPING) != 0)
        return DIGI_RET_ERROR;

    if (uiCurrentTime > pstCurrentDigimon->uiTimeSleep)
        return DIGI_RET_OK;
    if (uiCurrentTime > pstCurrentDigimon->uiTimeWakeUp)
        return DIGI_RET_ERROR;
    return DIGI_RET_OK;
}

uint8_t DIGI_shouldWakeUp(const playing_digimon_t* pstPlayingDigimon) {
    const digimon_t* pstCurrentDigimon = pstPlayingDigimon->pstCurrentDigimon;
    const uint16_t uiCurrentTime = guiCurrentTime;

    if (uiCurrentTime >= pstCurrentDigimon->uiTimeWakeUp &&
        uiCurrentTime < pstCurrentDigimon->uiTimeSleep &&
        (pstPlayingDigimon->uiStats & MASK_SLEEPING) != 0)
        return DIGI_RET_OK;
    return DIGI_RET_ERROR;
}

uint8_t DIGI_setCalled(playing_digimon_t* pstPlayingDigimon) {
    pstPlayingDigimon->uiStats &= ~MASK_CALLED;

    if (pstPlayingDigimon->pstCurrentDigimon->uiStage == DIGI_STAGE_EGG) {
        LOG("Impossible for egg to need for care, not calling");
        return DIGI_RET_ERROR;
    }

    if (GET_HUNGER_VALUE(pstPlayingDigimon->uiHungerStrength) == 0) {
        pstPlayingDigimon->uiStats |= MASK_CALLED;
        LOG("Will be calling because of hunger");
    } else if (GET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength) == 0) {
        pstPlayingDigimon->uiStats |= MASK_CALLED;
        LOG("Will be calling because of strength");
    } else if (DIGI_shouldSleep(pstPlayingDigimon) == DIGI_RET_OK) {
        pstPlayingDigimon->uiStats |= MASK_CALLED;
        LOG("Will be calling because of sleep");
    }

    if (pstPlayingDigimon->uiStats & MASK_CALLED)
        return DIGI_RET_OK;

    pstPlayingDigimon->iTimeBeingCalled = NOT_COUNTING_FOR_CARE_MISTAKE;
    return DIGI_RET_ERROR;
}

void DIGI_addCareMistakes(playing_digimon_t* pstPlayingDigimon) {
    LOG("HungerStrength %04x, shouldSleep() == %d",
        pstPlayingDigimon->uiHungerStrength,
        DIGI_shouldSleep(pstPlayingDigimon));
    if (GET_HUNGER_VALUE(pstPlayingDigimon->uiHungerStrength) == 0)
        pstPlayingDigimon->uiCareMistakesCount++;
    else if (GET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength) == 0)
        pstPlayingDigimon->uiCareMistakesCount++;
    else if (DIGI_shouldSleep(pstPlayingDigimon) == DIGI_RET_OK)
        pstPlayingDigimon->uiCareMistakesCount++;
}

uint8_t DIGI_poop(playing_digimon_t* pstPlayingDigimon, uint8_t uiAmount) {
    LOG("Pooping %d times (current amount %d)", uiAmount,
        pstPlayingDigimon->uiPoopCount);

    pstPlayingDigimon->uiPoopCount += uiAmount;
    if (pstPlayingDigimon->uiPoopCount >= 4) {
        pstPlayingDigimon->uiPoopCount = 4;
        pstPlayingDigimon->uiStats |= MASK_SICK;
        LOG("%s got sick from too much poop",
            pstPlayingDigimon->pstCurrentDigimon->szName);
        return DIGI_RET_SICK;
    }

    return DIGI_RET_OK;
}

void DIGI_cleanPoop(playing_digimon_t* pstPlayingDigimon) {
    LOG("Cleaning %d poops", pstPlayingDigimon->uiPoopCount);
    pstPlayingDigimon->uiPoopCount = 0;
}

uint8_t DIGI_proccesCalling(playing_digimon_t* pstPlayingDigimon,
                            uint8_t uiTimePassed) {
    if (pstPlayingDigimon->iTimeBeingCalled >= TIME_TO_GET_CARE_MISTAKE + 1) {
        LOG("No response in last call");
        return DIGI_RET_NOTHING;
    }

    pstPlayingDigimon->iTimeBeingCalled += uiTimePassed;

    if (pstPlayingDigimon->iTimeBeingCalled >= TIME_TO_GET_CARE_MISTAKE + 1) {
        LOG("Enough time has passed without calling, increasing care mistakes");
        DIGI_addCareMistakes(pstPlayingDigimon);
        LOG("Current count -> %d", pstPlayingDigimon->uiCareMistakesCount);
        return DIGI_RET_CARE_MISTAKE;
    }

    LOG("%d minutes have passed without care",
        pstPlayingDigimon->iTimeBeingCalled - 1);
    return DIGI_RET_OK;
}

uint8_t DIGI_shouldBeKilledOff(const playing_digimon_t* pstPlayingDigimon) {
    if (pstPlayingDigimon->uiCareMistakesCount >= 20) {
        LOG("[DIGILIB] Too many care mistakes count (%d)",
            pstPlayingDigimon->uiCareMistakesCount);
        return DIGI_RET_OK;
    }
    if (pstPlayingDigimon->uiInjuredCount >= 20) {
        LOG("[DIGILIB] Too many times injured (%d)",
            pstPlayingDigimon->uiInjuredCount);
        return DIGI_RET_OK;
    }
    if (pstPlayingDigimon->uiSickCount >= 20) {
        LOG("[DIGILIB] Too many times sick(%d)",
            pstPlayingDigimon->uiSickCount);
        return DIGI_RET_OK;
    }

    // If it is on a dying stage, only 5 care mistakes are needed for death.
    if ((pstPlayingDigimon->uiStats & MASK_DYING_STAGE) != 0 &&
        pstPlayingDigimon->uiCareMistakesCount >= 5) {
        LOG("[DIGILIB] Too many care mistakes while dying (%d)",
            pstPlayingDigimon->uiCareMistakesCount);
        return DIGI_RET_OK;
    }

    if (pstPlayingDigimon->uiTimeSickOrInjured >= 360) {
        LOG("[DIGILIB] Too much time injured/sick (%d)",
            pstPlayingDigimon->uiTimeSickOrInjured);
        return DIGI_RET_OK;
    }

    return DIGI_RET_ERROR;
}

uint16_t DIGI_timeToGetHungry(const playing_digimon_t* pstPlayingDigimon) {
    if (pstPlayingDigimon->uiTimedFlags & DIGI_TIMEDFLG_CAN_OVERFEED)
        return TIME_TO_GET_HUNGRY;

    LOG("Digimon got overfed, it will take twice as long to get hungry again "
        "(%d)",
        TIME_TO_GET_HUNGRY << 1);
    return TIME_TO_GET_HUNGRY << 1;
}

uint8_t DIGI_updateSleepDisturbance(playing_digimon_t* pstPlayingDigimon,
                                    uint16_t uiDeltaTime) {
    if (pstPlayingDigimon->pstCurrentDigimon->uiStage == DIGI_STAGE_EGG)
        return DIGI_RET_ERROR;

    if ((pstPlayingDigimon->uiTimedFlags & DIGI_TIMEDFLG_CAN_DISTURB_SLEEP)) {
        pstPlayingDigimon->uiTimeSinceSleepDisturbance = 0;
        return DIGI_RET_ERROR;
    }

    pstPlayingDigimon->uiTimeSinceSleepDisturbance += uiDeltaTime;
    if (pstPlayingDigimon->uiTimeSinceSleepDisturbance <
        TIME_TO_GET_CARE_MISTAKE)
        return DIGI_RET_OK;

    pstPlayingDigimon->uiTimedFlags |= DIGI_TIMEDFLG_CAN_DISTURB_SLEEP;
    return DIGI_RET_ERROR;
}
