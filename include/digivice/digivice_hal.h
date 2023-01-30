#ifndef DIGIVICE_HAL_H
#define DIGIVICE_HAL_H

#include "digitype.h"

typedef struct digivice_hal_t {
    void (*setLCDStatus)(uint8_t x, uint8_t y, uint8_t uiStatus);
    void (*render)();
} digivice_hal_t;

extern digivice_hal_t stDigiviceHal;

#endif // DIGIVICE_HAL_H