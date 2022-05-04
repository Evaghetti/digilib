#ifndef DIGIHARDWARE_H
#define DIGIHARDWARE_H

#include <stdint.h>

uint16_t DIGIHW_setTime();

uint16_t DIGIHW_timeMinutes();

void DIGIHW_addTime(uint16_t uiAddingAmount);

int16_t DIGIHW_readFile(const char* szFileName, void* pbDest,
                        const uint16_t uiMaxDestSize);

uint16_t DIGIHW_saveFile(const char* szFileName, const void* pbData,
                         const uint16_t uiSizeData);

#endif