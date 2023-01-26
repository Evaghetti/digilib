#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "include/digiapi.h"
#include "include/digihal.h"
#include "include/digitype.h"
#include "include/enums.h"

#ifdef __linux__
#include <unistd.h>
#define sleep(x) usleep(x * 1000)
#elif _WIN32
#include <windows.h>
#define sleep(x) Sleep(sleepMs)
#endif

size_t getRandom() {
    srand(time(NULL));
    return rand() % 16;
}

size_t getTime() {
    return time(NULL);
}

void* alloc(size_t i) {
    return malloc(i);
}

int32_t save(const void* data, size_t size) {
    FILE* file = fopen("save.data", "wb");
    if (file == NULL)
        return 0;

    int32_t ret = fwrite(data, 1, size, file);
    fclose(file);
    return ret;
}

int32_t load(void* data, size_t size) {
    FILE* file = fopen("save.data", "rb");
    if (file == NULL)
        return 0;

    int32_t ret = fread(data, 1, size, file);
    fclose(file);
    return ret;
}

digihal_t hal = {
    .malloc = alloc,
    .free = free,
    .log = printf,
    .randomNumber = getRandom,
    .getTimeStamp = getTime,
    .readData = load,
    .saveData = save,
};

int main() {
    uint8_t events, ret;
    playing_digimon_t* player;
    // Init HAL
    ret = DIGI_init(hal, &player);
    if (player == NULL) {
        fprintf(stderr, "Error Initializing");
        return 1;
    }

    // Select first digitama by default
    if (ret == 11 && DIGI_selectDigitama(player, 0) != 0) {
        fprintf(stderr, "Not possible to select digitama");
        return 1;
    }

    while (1) {
        system("clear");

        DIGI_updateEventsDeltaTime(player, 1, &events);
        printf("Events %d\n", events);
        printf("F) Food\n");
        printf("V) Vitamin\n");
        printf("C) Clean\n");
        printf("H) Heal\n");
        printf("S) Sleep\n");

        char op;
        scanf("%c", &op);
        switch (op) {
            case 'F':
            case 'f':
                ret = DIGI_feedDigimon(player, 1);
                break;
            case 'V':
            case 'v':
                ret = DIGI_stregthenDigimon(player, 1, 4);
                break;
            case 'C':
            case 'c':
                DIGI_cleanPoop(player);
                break;
            case 'H':
            case 'h':
                ret = DIGI_healDigimon(player, MASK_SICK | MASK_INJURIED);
                break;
            case 'S':
            case 's':
                ret = DIGI_putSleep(player, 1);
            default:
                printf("Invalid\n");
                break;
        }
        printf("Result %d\n", ret);
    }

    return 0;
}