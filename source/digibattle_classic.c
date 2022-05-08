#include "digibattle_classic.h"
#include "digicomm.h"
#include "digimon.h"

#include <stdio.h>

extern playing_digimon_t stPlayingDigimon;

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
    // TODO: Check slot table to define winner
    uint8_t uiResult = DIGIBATTLE_RET_WIN;

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
