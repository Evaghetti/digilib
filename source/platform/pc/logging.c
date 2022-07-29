#include "logging.h"

#ifndef NDEBUG
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

extern const char* gszSaveFile;

void addLog(const char* szFmt, ...) {
    char logFile[256];
    snprintf(logFile, sizeof(logFile), "%s.log", gszSaveFile);

    FILE* fileLog = fopen(logFile, "a+");
    if (fileLog == NULL)
        return;

    va_list args;
    va_start(args, szFmt);
    vfprintf(fileLog, szFmt, args);
    va_end(args);

    fclose(fileLog);
}

const char* getTime() {
    time_t currentTime = time(NULL);
    struct tm* timeInfo = localtime(&currentTime);

    static char szDate[50];
    strftime(szDate, sizeof(szDate), "%Y-%m-%d - %H:%M:%S", timeInfo);
    return szDate;
}

const char* getSaveFile() {
    return gszSaveFile;
}
#endif