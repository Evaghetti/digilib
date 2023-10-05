#ifndef DIGIVICE_HAL_H
#define DIGIVICE_HAL_H

#include "digitype.h"

typedef struct digivice_hal_t {
    void (*setLCDStatus)(uint8_t x, uint8_t y, uint8_t uiStatus);
    void (*setIconStatus)(uint8_t uiIndex, uint8_t uiStatus);
    void (*render)();
    size_t (*getTimeStamp)();

    size_t (*saveData)(const void* pData, size_t size);
    size_t (*readData)(void* pData, size_t size);
} digivice_hal_t;

extern const digivice_hal_t* gpstDigiviceHal;

uint8_t DIGIVICE_saveData(const void* pData, size_t size);
uint8_t DIGIVICE_readData(void* pData, size_t size);

#endif  // DIGIVICE_HAL_H
