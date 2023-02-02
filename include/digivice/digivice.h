#ifndef DIGIVICE_H
#define DIGIVICE_H

#include "digihal.h"
#include "digitype.h"
#include "digivice_hal.h"

uint8_t DIGIVICE_init(const digihal_t* pstHal,
                      const digivice_hal_t* pstDigiviceHal,
                      size_t uiFrequency);

uint8_t DIGIVICE_update();

#endif  // DIGIVICE_H