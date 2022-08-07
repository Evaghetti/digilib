#include "digivice/w0rld.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digiapi.h"
#include "digibattle_classic.h"
#include "digiworld.h"

#include <SDL.h>

#define SIZE_TOKEN        4
#define REPLACEABLE_TOKEN "DATA"

static char** tokens;
static int currentToken = 0, countTokens;

static char** tokenize(char* untokenizedStr, char* delim) {
    int mode = 0;
    char** tokens = calloc(sizeof(char*), 50);

    countTokens = 0;
    char* currentTokenStr = strtok(untokenizedStr, delim);
    while (currentTokenStr != NULL) {
        char** tokenOffset = &tokens[countTokens];
        *tokenOffset = calloc(sizeof(char), SIZE_TOKEN + 1);

        if (mode != 0) {

            int shouldAddData =
                mode == 2 ? countTokens % 2 != 0 : countTokens % 2 == 0;
            if (shouldAddData) {
                strncpy(*tokenOffset, REPLACEABLE_TOKEN, SIZE_TOKEN + 1);
                countTokens += 1;

                tokenOffset = &tokens[countTokens];
                *tokenOffset = calloc(sizeof(char), SIZE_TOKEN + 1);
            }

        } else {
            mode = currentTokenStr[1] == '2' ? 2 : 1;
        }

        strncpy(*tokenOffset, currentTokenStr, SIZE_TOKEN);
        countTokens += 1;

        currentTokenStr = strtok(NULL, delim);
    }

    currentToken = 1;
    return tokens;
}

static void freeTokens() {
    int i;

    for (i = 0; i < countTokens; i++)
        free(tokens[i]);
    free(tokens);
}

static char* detokenize() {
    static char result[100];
    char* currentPosition = result;
    int i, mode = 0;

    memset(result, 0, sizeof(result));
    for (i = 0; i < countTokens; i++) {
        if (mode) {
            if (mode == 1) {
                if (i % 2 != 0)
                    strcpy(currentPosition, "s:");
                else
                    strcpy(currentPosition, "r:");
            } else {
                if (i % 2 == 0)
                    strcpy(currentPosition, "s:");
                else
                    strcpy(currentPosition, "r:");
            }
            currentPosition += 2;
        } else {
            mode = tokens[i][1] == '2' ? 2 : 1;
            continue;
        }

        strcpy(currentPosition, tokens[i]);
        currentPosition += strlen(tokens[i]);
        if (i < countTokens - 1)
            *currentPosition = ' ';
        else
            *currentPosition = '\n';
        currentPosition++;
    }

    return result;
}

static void sendData(const char* data, int sizeData) {
    SDL_Log("%s", data);
    if (writeDataDCOM(data, sizeData) == -1)
        SDL_Log("Error sending data to socket");
}

static uint16_t sendCallBack(uint16_t dataToSend) {
    int swapped = 0;
    for (; currentToken < countTokens; currentToken++) {
        if (strcmp(tokens[currentToken], REPLACEABLE_TOKEN) == 0) {
            snprintf(tokens[currentToken], SIZE_TOKEN + 1, "%04x", dataToSend);
            swapped = 1;
            currentToken++;
            break;
        }
    }

    // If no more swaps happen, just mirror.
    if (!swapped) {
        tokens[countTokens] = calloc(sizeof(char), SIZE_TOKEN + 1);
        snprintf(tokens[countTokens], SIZE_TOKEN + 1, "%04x", dataToSend);

        countTokens++;
    }

    return DIGIBATTLE_RET_OK;
}

static uint16_t recvCallBack() {
    uint16_t ret;
    sscanf(tokens[currentToken], "%hx", &ret);
    currentToken++;
    return ret;
}

int doBattleWithDCOM() {
    char input[100] = {0}, bufferOutput[200] = {0};
    int result = 0;

    if (!prepareDCOMLogic()) {
        SDL_Log("Error while preparing DCOM");
        return 5;
    }

    readDataDCOM(input, sizeof(input));
    if (strlen(input) == 0) {
        return 0;
    }

    char* newLine = strstr(input, "\n");
    if (newLine != NULL)
        *newLine = '\0';

    char originalInput[sizeof(input)] = {0};

    memcpy(originalInput, input, strlen(input));
    tokens = tokenize(input, "-");

    snprintf(bufferOutput, sizeof(bufferOutput),
             "got %ld bytes: %s  -> %s-{%d packets}\n", strlen(originalInput),
             originalInput, tokens[0], countTokens - 1);

    sendData(bufferOutput, strlen(bufferOutput));
    if (strcmp(tokens[1], "0000") == 0) {
        freeTokens();
        SDL_Log("Initializiation");
        return 3;  // Force error when initializing.
    }

    if (tokens[0][1] == '2') {
        DIGIBATTLE_initiate(&sendCallBack, &recvCallBack);
        result = 3;
    } else {
        result = DIGIBATTLE_continue(&sendCallBack, &recvCallBack);
    }

    DIGIBATTLE_changeStats(result);

    char* simulatedDcom = detokenize();
    sendData(simulatedDcom, strlen(simulatedDcom));
    while (1) {
        SDL_Delay(500);
        readDataDCOM(input, sizeof(input));
        if (strlen(input) > 0) {
            SDL_Log("Ignored %s", input);
            break;
        }
    }
    freeTokens();
    SDL_Log("Result -> %d", result);
    return result;
}