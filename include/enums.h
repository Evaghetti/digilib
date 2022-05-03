#ifndef ENUM_H
#define ENUM_H

// Possible returns
#define DIGI_RET_OK       0
#define DIGI_RET_ERROR    (DIGI_RET_OK + 1)
#define DIGI_NO_EVOLUTION (DIGI_RET_OK + 2)
#define DIGI_RET_OVERFEED (DIGI_RET_OK + 3)
#define DIGI_RET_HUNGRY   (DIGI_RET_OK + 4)
#define DIGI_RET_WEAK     (DIGI_RET_OK + 5)
#define DIGI_RET_SICK     (DIGI_RET_OK + 6)

// Masks representing possible events that happen at each update
#define DIGI_EVENT_MASK_INJURED 0b00000001
#define DIGI_EVENT_MASK_SICK    0b00000010
#define DIGI_EVENT_MASK_EVOLVE  0b00000100
#define DIGI_EVENT_MASK_DIE     0b00001000
#define DIGI_EVENT_MASK_CALL    0b10000000

// Attributes that each digimon can have
#define DIGI_ATTRIBUTE_FREE    0
#define DIGI_ATTRIBUTE_DATA    1
#define DIGI_ATTRIBUTE_VACCINE 2
#define DIGI_ATTRIBUTE_VIRUS   3

#define DIGI_STAGE_EGG            0
#define DIGI_STAGE_BABY_1         1
#define DIGI_STAGE_BABY_2         2
#define DIGI_STAGE_CHILD          3
#define DIGI_STAGE_ADULT          4
#define DIGI_STAGE_PERFECT        5
#define DIGI_STAGE_ULTIMATE       6
#define DIGI_STAGE_SUPER_ULTIMATE 7

#endif