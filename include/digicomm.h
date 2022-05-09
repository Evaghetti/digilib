#ifndef DIGICOMM_H
#define DIGICOMM_H

#include <stdint.h>

#define DIGICOMM_OK            0
#define DIGICOMM_ERROR_POLLING (uint8_t)(DIGICOMM_OK - 1)
#define DIGICOMM_ERROR_READING (uint8_t)(DIGICOMM_OK - 2)
#define DIGICOMM_ERROR_WRITING (uint8_t)(DIGICOMM_OK - 3)

uint16_t DIGICOMM_setup();

uint16_t DIGICOMM_pollData();

uint16_t DIGICOMM_sendData(uint16_t uiData);

void DIGICOMM_close();

#endif