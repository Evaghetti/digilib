#include <stdio.h>
#include <time.h>

#include "digiapi.h"
#include "digimon.h"
#include "enums.h"

evolution_requirement_t vstPossibleRequirements[] = {
    {0b00000000},
    {MASK_NEEDS_CARE_MISTAKES, 0x0003},
    {MASK_NEEDS_CARE_MISTAKES | MASK_NEEDS_TRAINING, 0x0003, 0x20FF},
    {
        MASK_NEEDS_WIN_COUNT,
        0x0000,
        0x0000,
        0x0000,
        0x0000,
        0x0C0F,
    }};

digimon_t vstPossibleDigimon[] = {
    {
        "Digitama 1",  // Nome
        0b0000,        // Slot
        0x0000,        // Hora que acorda
        0x0000,        // Hora que dorme
        0x00,          // Digital monster não tem atributo, só slot
        5,             // Quanto tempo para evoluir
        DIGI_STAGE_EGG,
        1,                              // Quantas possiveis evoluções,
        {&vstPossibleRequirements[0]},  // Requesitos para evoluir.
        {&vstPossibleDigimon[1]}        // Digimons que evolui
    },
    {
        "Botamon",  // Nome
        0b0001,     // Slot
        0x0000,     // Hora que acorda
        0x2359,     // Hora que dorme
        0x00,       // Digital monster não tem atributo, só slot
        5,          // Quanto tempo para evoluir
        DIGI_STAGE_BABY_1,
        1,                              // Quantas possiveis evoluções
        {&vstPossibleRequirements[0]},  // Requesitos para evoluir.
        {&vstPossibleDigimon[2]}        // Digimons que evolui
    },
    {
        "Koromon",  // Nome
        0b0010,     // Slot
        0x0800,     // Hora que acorda
        0x1900,     // Hora que dorme
        0x00,       // Digital monster não tem atributo, só slot
        5,          // Quanto tempo para evoluir
        DIGI_STAGE_BABY_2,
        1,                              // Quantas possiveis evoluções
        {&vstPossibleRequirements[1]},  // Requesitos para evoluir.
        {&vstPossibleDigimon[3]}        // Digimons que evolui
    },
    {
        "Agumon",  // Nome
        0b0011,    // Slot
        0x0800,    // Hora que acorda
        0x2100,    // Hora que dorme
        0x00,      // Digital monster não tem atributo, só slot
        5,         // Quanto tempo para evoluir
        DIGI_STAGE_CHILD,
        1,                              // Quantas possiveis evoluções
        {&vstPossibleRequirements[2]},  // Requesitos para evoluir.
        {&vstPossibleDigimon[4]}        // Digimons que evolui
    },
    {
        "Greymon",  // Nome
        0b0101,     // Slot
        0x0800,     // Hora que acorda
        0x2000,     // Hora que dorme
        0x00,       // Digital monster não tem atributo, só slot
        5,          // Quanto tempo para evoluir
        DIGI_STAGE_ADULT,
        1,                              // Quantas possiveis evoluções
        {&vstPossibleRequirements[3]},  // Requesitos para evoluir.
        {&vstPossibleDigimon[5]}        // Digimons que evolui
    },
    {
        "Metal Greymon",  // Nome
        0b1100,           // Slot
        0x0800,           // Hora que acorda
        0x2100,           // Hora que dorme
        0x00,             // Digital monster não tem atributo, só slot
        5,                // Quanto tempo para evoluir
        DIGI_STAGE_PERFECT,
        0,   // Quantas possiveis evoluções
        {},  // Requesitos para evoluir.
        {}   // Digimons que evolui
    }};

int main() {
    char option;

    DIGI_init("ENZO");

    while (1) {
        uint8_t uiEvents, uiRet;
        printf(
            "F) Feed\nS) Strengthen\nI) Heal Injury\nH) Heal Sickness\nC) "
            "Clean "
            "Waste\n");

        if (scanf("%c", &option) == EOF)
            break;

        uiRet = DIGI_updateEventsDeltaTime(1, &uiEvents);
        if (uiRet != DIGI_RET_OK) {
            printf("[DIGILIB] Error during proccessing of events: 0x%x\n",
                   uiRet);
            return 1;
        }

        if (uiEvents & DIGI_EVENT_MASK_CALL)
            printf("[DIGILIB] Digimon is calling for you!\n");

        switch (option) {
            case 'F':
            case 'f':
                printf("[DIGITEST] Result Feeding: %d\n", DIGI_feedDigimon(1));
                break;
            case 's':
            case 'S':
                printf("[DIGITEST] Result Strength: %d\n",
                       DIGI_stregthenDigimon(1));
                break;
            case 'i':
            case 'I':
                printf("[DIGITEST] Result healing: %d\n",
                       DIGI_healDigimon(MASK_INJURIED));
                break;
            case 'h':
            case 'H':
                printf("[DIGITEST] Result healing: %d\n",
                       DIGI_healDigimon(MASK_SICK));
                break;
            case 'c':
            case 'C':
                DIGI_cleanWaste();
                break;

            default:
                break;
        }
    }

    return 0;
}
