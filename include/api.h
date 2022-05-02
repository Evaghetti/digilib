#ifndef API_H
#define API_H

#define DIGI_EVENT_MASK_INJURED 0b00000001
#define DIGI_EVENT_MASK_SICK    0b00000010
#define DIGI_EVENT_MASK_EVOLVE  0b00000100
#define DIGI_EVENT_MASK_DIE     0b00001000
#define DIGI_EVENT_MASK_CALL    0b10000000

#include "digimon.h"

uint8_t DIGI_updateEventsDeltaTime(uint16_t uiDeltaTime, uint8_t* puiEvents,
                                   playing_digimon_t* pstPlayingDigimon);

#endif  // API_H