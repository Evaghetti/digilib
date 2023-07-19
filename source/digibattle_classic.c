#include "digibattle_classic.h"
#include "digihal.h"
#include "enums.h"

#define AMOUNT_WEIGHT_REDUCE 4

#include <stdio.h>

static const uint8_t vuiChanceTable[12][12] = {
    {8, 8, 2, 3, 2, 3, 2, 3, 7, 1, 1, 1},
    {8, 8, 2, 3, 2, 3, 2, 3, 7, 1, 1, 1},
    {15, 15, 8, 11, 9, 11, 7, 11, 13, 3, 3, 3},
    {13, 13, 5, 8, 5, 9, 5, 7, 11, 2, 2, 2},
    {15, 15, 7, 11, 8, 11, 9, 11, 13, 3, 3, 3},
    {13, 13, 5, 7, 5, 8, 5, 9, 11, 2, 2, 2},
    {15, 15, 9, 11, 7, 11, 8, 11, 13, 3, 3, 3},
    {13, 13, 5, 9, 5, 7, 5, 8, 11, 2, 2, 2},
    {9, 9, 3, 5, 3, 5, 3, 5, 8, 1, 1, 1},
    {15, 15, 13, 14, 13, 14, 13, 14, 15, 8, 5, 5},
    {15, 15, 13, 14, 13, 14, 13, 14, 15, 11, 8, 5},
    {15, 15, 13, 14, 13, 14, 13, 14, 15, 11, 11, 8}};

uint16_t DIGIBATTLE_createFirstPacket(
    const playing_digimon_t* pstPlayingDigimon) {
    uint16_t uiPacket;

    // TODO: Find out what affects the value of effort, using by default the max value
    uiPacket =
        (0b0100 << 4) | pstPlayingDigimon->pstCurrentDigimon->uiSlotPower;
    uiPacket = (~uiPacket << 8) | uiPacket;

    LOG("First packet generated -> 0x%04x", uiPacket);
    return uiPacket;
}

uint16_t DIGIBATTLE_createSecondPacket(
    const playing_digimon_t* pstPlayingDigimon, uint8_t uiResult) {
    uint16_t uiPacket;

    uiPacket =
        (pstPlayingDigimon->pstCurrentDigimon->uiVersion << 4) | uiResult;
    uiPacket = (~uiPacket << 8) | uiPacket;

    LOG("Second packet generated -> 0x%04x", uiPacket);
    return uiPacket;
}

uint8_t DIGIBATTLE_getBattleResult(uint8_t uiMySlot, uint8_t uiEnemySlot) {
    uint8_t uiDiceRoll = gpstHal->randomNumber();

    uiMySlot -= 3;
    uiEnemySlot -= 3;

    return uiDiceRoll <= vuiChanceTable[uiMySlot][uiEnemySlot]
               ? DIGIBATTLE_RET_WIN
               : DIGIBATTLE_RET_LOSE;
}

uint8_t isValidPacket(uint16_t uiPacket) {
    uint8_t realValue = uiPacket & 0b11111111;
    uint8_t reverseValue = ~(uiPacket >> 8);

    return realValue == reverseValue ? DIGIBATTLE_RET_OK : DIGIBATTLE_RET_ERROR;
}

uint8_t DIGIBATTLE_canBattle(const playing_digimon_t* pstPlayingDigimon) {
    if (pstPlayingDigimon->pstCurrentDigimon->uiStage <= DIGI_STAGE_BABY_2) {
        LOG("Digimon is too young to fight");
        return DIGIBATTLE_RET_ERROR;
    }

    if ((pstPlayingDigimon->uiStats & MASK_SLEEPING) != 0 ||
        DIGI_shouldSleep(pstPlayingDigimon) == DIGI_RET_OK) {
        LOG("Digimon is too tired to battle");
        return DIGIBATTLE_RET_ERROR;
    }

    if (GET_HUNGER_VALUE(pstPlayingDigimon->uiHungerStrength) == 0 ||
        GET_STRENGTH_VALUE(pstPlayingDigimon->uiHungerStrength) == 0) {
        LOG("[DIGILIB] You have to take care of you digimon before battle");
        return DIGIBATTLE_RET_ERROR;
    }

    return DIGIBATTLE_RET_OK;
}

void DIGIBATTLE_changeStats(playing_digimon_t* pstPlayingDigimon,
                            uint8_t uiResultBattle) {
    if ((uiResultBattle & (DIGIBATTLE_RET_WIN | DIGIBATTLE_RET_LOSE)) == 0)
        return;

    pstPlayingDigimon->uiBattleCount++;
    if (uiResultBattle == DIGIBATTLE_RET_WIN)
        pstPlayingDigimon->uiWinCount++;

    pstPlayingDigimon->uiWeight -= AMOUNT_WEIGHT_REDUCE;
}

uint8_t DIGI_battle(playing_digimon_t* pstPlayingDigimon, uint8_t uiInitiate) {
    if (DIGIBATTLE_canBattle(pstPlayingDigimon) != DIGIBATTLE_RET_OK)
        return DIGIBATTLE_RET_OK;

    // First, see if the other side already has started communcating.
    // Then if it didn't, and this side should initiate, do so
    uint8_t uiResult = DIGIBATTLE_continue(pstPlayingDigimon);
    if (uiResult == DIGIBATTLE_RET_ERROR && uiInitiate) {
        uiResult = DIGIBATTLE_initiate(pstPlayingDigimon);
    }

    DIGIBATTLE_changeStats(pstPlayingDigimon, uiResult);
    return uiResult;
}

uint8_t DIGIBATTLE_initiate(playing_digimon_t* pstPlayingDigimon) {
    LOG("Initiating battle");

    uint16_t uiPacket = DIGIBATTLE_createFirstPacket(pstPlayingDigimon);
    LOG("Challenger: First packet sent -> 0x%04x", uiPacket);
    if (gpstHal->send(uiPacket)) {
        LOG("Challenger: Error trying to send data 0x%04x", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    uiPacket = gpstHal->recv();
    if (isValidPacket(uiPacket) != DIGIBATTLE_RET_OK) {
        LOG("Challenger: Received packet %x isn't valid", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    uint8_t uiEnemySlot = uiPacket & 0b1111;
    uint8_t uiResult = DIGIBATTLE_getBattleResult(
        pstPlayingDigimon->pstCurrentDigimon->uiSlotPower, uiEnemySlot);

    LOG("[DIGILIB] Challenger: Data got -> 0x%04x, Enemy Slot -> 0x%04x, "
        "Result -> %d",
        uiPacket, uiEnemySlot, uiResult);

    uiPacket = DIGIBATTLE_createSecondPacket(pstPlayingDigimon, uiResult);
    LOG("Challenger: Sending packet %04x", uiPacket);
    if (gpstHal->send(uiPacket)) {
        LOG("Challenger: Error trying to send last data 0x%04x", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    uiPacket = gpstHal->recv();
    LOG("Challenger: Received last package %04x", uiPacket);
    if (isValidPacket(uiPacket) != DIGIBATTLE_RET_OK) {
        LOG("Challenger: Second received packet %x isn't valid", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    return uiResult;
}

uint8_t DIGIBATTLE_continue(playing_digimon_t* pstPlayingDigimon) {
    LOG("Challenged: Reading data to continue");
    uint16_t uiPacket = gpstHal->recv();
    if (isValidPacket(uiPacket) != DIGIBATTLE_RET_OK) {
        LOG("Challenged: Received packet %x isn't valid", uiPacket);
        return uiPacket;
    }

    LOG("Challenged: Got data -> 0x%04x, Sending first packet", uiPacket);
    if (gpstHal->send(DIGIBATTLE_createFirstPacket(pstPlayingDigimon))) {
        LOG("Challenged: Error trying to send data 0x%04x", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    LOG("Challenged: waiting for second package");
    uiPacket = gpstHal->recv();
    if (isValidPacket(uiPacket) != DIGIBATTLE_RET_OK) {
        LOG("Challenged: Second received packet %x isn't valid", uiPacket);
        return uiPacket;
    }
    LOG("Challenged: Second packet %04x, but will be ignored", uiPacket);
    // Get inverted result (if the other one won, we lose and vice versa)
    uint8_t uiResult = ~uiPacket & 0b11;
    // Doesn't matter what the other side does with this
    LOG("Challenged: Result -> %d", uiResult);
    gpstHal->send(DIGIBATTLE_createSecondPacket(pstPlayingDigimon, uiResult));
    return uiResult;
}
