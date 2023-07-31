#ifndef DIGIHAL_H
#define DIGIHAL_H

#include <stdarg.h>
#include "digitype.h"

#ifndef NDEBUG

#define LOG(x, ...)                          \
    if (gpstHal && gpstHal->log) {           \
        gpstHal->log(x "\n", ##__VA_ARGS__); \
    }

#else

#define LOG(fmt, ...)

#endif  // NDGEBUG

typedef struct digihal_t {
    void* (*malloc)(size_t size);
    void (*free)(void* pBuff);
    int32_t (*log)(const char* buff, ...);

    size_t (*saveData)(const void* pData, size_t size);
    size_t (*readData)(void* pData, size_t size);
    uint8_t (*randomNumber)();

    uint16_t (*send)(uint16_t);
    uint16_t (*recv)();

    uint16_t (*getTime)();
} digihal_t;

extern const digihal_t* gpstHal;

uint8_t DIGI_setHal(const digihal_t* pstConfig);

#endif  // DIGIHAL_H
