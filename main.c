#include <stdio.h>
#include <time.h>

#include "api.h"
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

static time_t currentTime = -1, lastTime = -1;
uint8_t proccessEvents(uint8_t* puiEvents,
                       playing_digimon_t* pstPlayingDigimon) {
    currentTime = time(NULL);
    if (lastTime == -1)
        lastTime = currentTime;

    uint16_t uiDeltaTime = (currentTime - lastTime) / 60;
    if (uiDeltaTime > 0)
        lastTime = currentTime;

    return DIGI_updateEventsDeltaTime(uiDeltaTime, puiEvents,
                                      pstPlayingDigimon);
}

int main() {
    playing_digimon_t stPlayingDigimon = {&vstPossibleDigimon[0], 0b00001111,
                                          0x00, 0x00, 0x00};
    char option;

    while (1) {
        uint8_t uiEvents, uiRet;
        printf("%s\nH: %d\nS: %d\n\n",
               stPlayingDigimon.pstCurrentDigimon->szName,
               GET_HUNGER_VALUE(stPlayingDigimon.uiStats),
               GET_STRENGTH_VALUE(stPlayingDigimon.uiStats));
        printf("E) evolve\nF) Feed\n");

        if (scanf("%c", &option) == EOF)
            break;

        uiRet = proccessEvents(&uiEvents, &stPlayingDigimon);
        if (uiRet != DIGI_RET_OK) {
            printf("[DIGILIB] Error during proccessing of events: 0x%x\n",
                   uiRet);
            return 1;
        }

        if (uiEvents & DIGI_EVENT_MASK_CALL)
            printf("[DIGILIB] Digimon is calling for you!\n");

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
    }

    return 0;
}
