#include <stdio.h>
#include "digimon.h"

evolution_requirement_t vstPossibleRequirements[] = {{0x3f, 0x0, 0x0a},
                                                     {0x3f, 0x0, 1200}};

digimon_t vstPossibleDigimon[] = {
    {"Botamon",
     0,
     2359,
     ATTRIBUTE_FREE,
     1,
     {&vstPossibleRequirements[0]},
     {&vstPossibleDigimon[1]}},
    {"Koromon",
     0,
     2100,
     ATTRIBUTE_FREE,
     1,
     {&vstPossibleRequirements[1]},
     {&vstPossibleDigimon[2]}},
    {"Agumon", 18, 2100, ATTRIBUTE_VACCINE, 0, {}, {}},
};

int main() {
    playing_digimon_t stPlayingDigimon = {&vstPossibleDigimon[0], 0x00, 0x00,
                                          0x00, 0x00};
    char option;
    sizeof(vstPossibleDigimon);

    printf("%s\nH: %d\nS: %d\n\n", stPlayingDigimon.pstCurrentDigimon->szName,
           GET_HUNGER_VALUE(stPlayingDigimon.uiStats),
           GET_STRENGTH_VALUE(stPlayingDigimon.uiStats));
    printf("E) evolve\nF) Feed\n");
    while (scanf("%c", &option) != EOF) {
        switch (option) {
            case 'E':
            case 'e': {
                int newForm = DIGI_evolveDigimon(&stPlayingDigimon);
                printf("[DIGITEST] Result evolution: %d\n", newForm);

                if (newForm >= 0)
                    stPlayingDigimon.pstCurrentDigimon =
                        stPlayingDigimon.pstCurrentDigimon
                            ->vstPossibleEvolutions[newForm];
            } break;

            case 'F':
            case 'f':
                printf("[DIGITEST] Result Feeding: %d\n",
                       DIGI_feedDigimon(&stPlayingDigimon, 1));
                break;

            default:
                break;
        }

        printf("%s\nH: %d\nS: %d\n\n",
               stPlayingDigimon.pstCurrentDigimon->szName,
               GET_HUNGER_VALUE(stPlayingDigimon.uiStats),
               GET_STRENGTH_VALUE(stPlayingDigimon.uiStats));
        printf("E) evolve\nF) Feed\n");
    }

    return 0;
}
