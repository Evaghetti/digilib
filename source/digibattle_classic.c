#include "digibattle_classic.h"
#include "digicomm.h"
#include "digihardware.h"
#include "digimon.h"

#include <stdio.h>

extern playing_digimon_t stPlayingDigimon;

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

uint16_t createFirstPacket() {
    uint16_t uiPacket;

    // TODO: Find out what affects the value of effort, using the default 0b0000 of
    // newer devices
    uiPacket = (0b0000 << 4) | stPlayingDigimon.pstCurrentDigimon->uiSlotPower;
    uiPacket = (~uiPacket << 8) | uiPacket;

    printf("[DIGILIB] First packet generated -> 0x%04x\n", uiPacket);
    return uiPacket;
}

uint16_t createSecondPacket(uint8_t uiResult) {
    uint16_t uiPacket;
    // By default using version 1.
    uiPacket = (0b0000 << 4) | uiResult;
    uiPacket = (~uiPacket << 8) | uiPacket;

    printf("[DIGILIB] Second packet generated -> 0x%04x\n", uiPacket);
    return uiPacket;
}

uint8_t getBattleResult(uint8_t uiMySlot, uint8_t uiEnemySlot) {
    uint8_t uiDiceRoll = DIGIHW_randomNumber();

    uiMySlot -= 3;
    uiEnemySlot -= 3;

    return uiDiceRoll <= vuiChanceTable[uiMySlot][uiEnemySlot]
               ? DIGIBATTLE_RET_WIN
               : DIGIBATTLE_RET_LOSE;
}

uint8_t DIGI_battle(uint8_t uiInitiate) {
    return 0;
}

uint8_t DIGIBATTLE_initiate() {
    uint16_t uiPacket = createFirstPacket();

    printf("[DIGILIB] First packet sent -> 0x%04x\n", uiPacket);
    if (DIGICOMM_sendData(uiPacket)) {
        printf("[DIGILIB] Error trying to send data 0x%04x\n", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    uiPacket = DIGICOMM_pollData();
    if (uiPacket == DIGICOMM_ERROR_POLLING ||
        uiPacket == DIGICOMM_ERROR_READING) {
        printf("[DIGILIB] Error receiving data\n");
        return DIGIBATTLE_RET_ERROR;
    }

    uint8_t uiEnemySlot = uiPacket & 0b1111;
    uint8_t uiResult = getBattleResult(
        stPlayingDigimon.pstCurrentDigimon->uiSlotPower, uiEnemySlot);

    printf("[DIGILIB] Data got -> 0x%04x, Enemy Slot -> 0x%04x, Result -> %d\n",
           uiPacket, uiEnemySlot, uiResult);

    uiPacket = createSecondPacket(uiResult);
    if (DIGICOMM_sendData(uiPacket)) {
        printf("[DIGILIB] Error trying to send last data 0x%04x\n", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    DIGICOMM_pollData();  // Last response from the other side does not matter.
    return uiResult;
}

uint8_t DIGIBATTLE_continue() {
    printf("[DIGILIB] Reading data to continue\n");
    uint16_t uiPacket = DIGICOMM_pollData();
    if (uiPacket == DIGICOMM_ERROR_POLLING ||
        uiPacket == DIGICOMM_ERROR_READING) {
        printf("[DIGILIB] Error receiving data\n");
        return DIGIBATTLE_RET_ERROR;
    }

    printf("[DIGILIB] Got data -> 0x%04x, Sending first packet\n", uiPacket);
    if (DIGICOMM_sendData(createFirstPacket())) {
        printf("[DIGILIB] Error trying to send data 0x%04x\n", uiPacket);
        return DIGIBATTLE_RET_ERROR;
    }

    printf("[DIGILIB] waiting for second package\n");
    uiPacket = DIGICOMM_pollData();
    if (uiPacket == DIGICOMM_ERROR_POLLING ||
        uiPacket == DIGICOMM_ERROR_READING) {
        printf("[DIGILIB] Error receiving data\n");
        return DIGIBATTLE_RET_ERROR;
    }

    // Get inverted result (if the other one won, we lose and vice versa)
    uint8_t uiResult = ~uiPacket & 0b11;
    // Doesn't matter what the other side does with this
    printf("[DIGILIB] Result -> %d\n", uiResult);
    DIGICOMM_sendData(createSecondPacket(uiResult));
    return uiResult;
}
