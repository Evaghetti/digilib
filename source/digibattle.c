#include "digibattle.h"
#include "digicomm.h"
#include "digimon.h"

#include <stdio.h>

extern playing_digimon_t stPlayingDigimon;

uint8_t DIGIBATTLE_initiate() {
    if (DIGICOMM_sendData(0x1337) != DIGICOMM_OK) {
        printf("[DIGILIB] Error initiating battle\n");
        return 0;
    }

    if (DIGICOMM_pollData() == DIGICOMM_ERROR_POLLING) {
        printf("[DIGILIB Error looking for data\n");
        return 0;
    }
}