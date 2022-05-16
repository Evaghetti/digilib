#include <stdio.h>
#include <time.h>

#include "digiapi.h"
#include "digibattle_classic.h"
#include "digicomm.h"
#include "digimon.h"
#include "enums.h"

int main() {
    char option;
    uint8_t sleeping = 0;

    if (DIGI_init("ENZO") != DIGI_RET_OK)
        DIGI_initDigitama("ENZO", 0);

    while (1) {
        uint8_t uiEvents, uiRet, uiInitiate = 0, uiTry = 0;
        printf(
            "F) Feed\nS) Strengthen\nI) Heal Injury\nH) Heal Sickness\nC) "
            "Clean "
            "Waste\nP) Put to sleep\nB) Initiate Battle\nL) Listen for "
            "Battle\n");

        fflush(stdin);
        option = getchar();
        printf("Selected option -> %c\n", option);

        uiRet = DIGI_updateEventsDeltaTime(1, &uiEvents);
        if (uiRet != DIGI_RET_OK) {
            printf("[DIGILIB] Error during proccessing of events: 0x%x\n",
                   uiRet);
            return 1;
        }

        if (uiEvents & DIGI_EVENT_MASK_CALL)
            printf("[DIGILIB] Digimon is calling for you!\n");

        do {
            switch (option) {
                case 'F':
                case 'f':
                    printf("[DIGITEST] Result Feeding: %d\n",
                           DIGI_feedDigimon(1));
                    break;
                case 's':
                case 'S':
                    printf("[DIGITEST] Result Strength: %d\n",
                           DIGI_stregthenDigimon(1, 2));
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
                case 'p':
                case 'P':
                    DIGI_putSleep(!sleeping);
                    break;
                case 'b':
                case 'B':
                    uiInitiate = 1;
                    // Fallthrough
                case 'l':
                case 'L':
                    uiRet = DIGI_battle(uiInitiate);
                    if (uiRet == DIGIBATTLE_RET_WIN ||
                        uiRet == DIGIBATTLE_RET_LOSE)
                        uiTry = 10;
                    uiTry++;
                    break;
                default:
                    break;
            }
        } while (uiTry < 5 && (option == 'b' || option == 'B' ||
                               option == 'l' || option == 'L'));
    }

    return 0;
}
