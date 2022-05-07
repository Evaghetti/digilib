#include "digicomm.h"
#include "digihardware.h"

#include <stdio.h>

#ifdef WIN32
#include <windows.h>
#else
#include <unistd.h>
#define Sleep(x) usleep((x)*1000)
#endif

#define SECONDS_WAITING_POOL 60
#define COMMUNICATION_FILE   "data.comm"

int16_t DIGICOMM_pollData() {
    uint16_t uiBytesRead = 0;
    uint8_t uiTries;

    for (uiTries = SECONDS_WAITING_POOL; uiTries && !uiBytesRead; uiTries--) {
        printf("[DIGILIB] Looking for data...\n");
        if (DIGIHW_readFile(COMMUNICATION_FILE, &uiBytesRead,
                            sizeof(uiBytesRead)) == sizeof(uiBytesRead)) {
            printf("[DIGILIB] Found! %04x\n", uiBytesRead);
            remove(COMMUNICATION_FILE);
            return uiBytesRead;
        }

        Sleep(1000);
    }

    return DIGICOMM_ERROR_POLLING;
}

int16_t DIGICOMM_sendData(uint16_t uiData) {
    uint8_t uiTries, uiBytesRead = 1;

    printf("[DIGLIB] Sending data %04x\n", uiData);
    DIGIHW_saveFile(COMMUNICATION_FILE, &uiData, sizeof(uiData));

    for (uiTries = SECONDS_WAITING_POOL; uiTries && uiBytesRead; uiTries--) {
        printf("[DIGILIB] Verifying for data being received by target...\n");
        if (DIGIHW_readFile(COMMUNICATION_FILE, &uiBytesRead,
                            sizeof(uiBytesRead)) == -1) {
            printf("[DIGILIB] Received!");
            return DIGICOMM_OK;
        }
        Sleep(1000);
    }

    return DIGICOMM_ERROR_WRITING;
}
