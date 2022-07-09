#include "digivice/battle.h"
#include "digibattle_classic.h"
#include "digivice/globals.h"
#include "digiworld.h"

#include "SDL2/SDL_net.h"

#define IP   "localhost"
#define PORT 1998

// States
typedef enum States {
    BATTLE_REGISTER,
    BATTLE_CHALLENGE,
    BATTLE_REQUEST,
    BATTLE_LIST,
} State;

// Tags TLV
typedef enum Tags {
    SLOT_POWER,
    VERSION,
    COUNT,
} Tag;

typedef struct {
    unsigned char tag;
    unsigned char length;
    void* value;
} TagLengthValue;

static TCPsocket connection = NULL;

int connectToServer() {
    // Already connected
    if (connection != NULL)
        return 1;

    IPaddress ip;

    if (SDLNet_ResolveHost(&ip, IP, PORT) == -1) {
        SDL_Log("Error resolving host %s:%d -> %s", IP, PORT,
                SDLNet_GetError());
        return 0;
    }

    connection = SDLNet_TCP_Open(&ip);
    if (connection == NULL) {
        SDL_Log("Error creating socket to host %s:%d -> %s", IP, PORT,
                SDLNet_GetError());
        return 0;
    }

    SDL_Log("Created socket");
    return 1;
}

static unsigned char* allocBufferCOMM(State state, TagLengthValue data[],
                                      int countData, int* sizeResult) {
    int i, offsetBuffer = 0;
    unsigned char* socketData = NULL;

    if (sizeResult == NULL) {
        SDL_Log("No way to output size of buffer");
        return NULL;
    }

    *sizeResult = 1;
    for (i = 0; i < countData; i++)
        *sizeResult += 2 + data[i].length;

    socketData = calloc(*sizeResult, sizeof(unsigned char));
    if (socketData == NULL) {
        SDL_Log("Error callocing data to be sent to server");
        return NULL;
    }

    socketData[offsetBuffer++] = state;
    for (i = 0; i < countData; i++) {
        socketData[offsetBuffer++] = data[i].tag;
        socketData[offsetBuffer++] = data[i].length;
        memcpy(socketData + offsetBuffer, data[i].value, data[i].length);
        offsetBuffer += data[i].length;
    }

    return socketData;
}

digimon_t* getDigimon(unsigned char slot, unsigned char version) {
    int i;
    for (i = 0; i < MAX_COUNT_DIGIMON; i++) {
        if (slot == vstPossibleDigimon[i].uiSlotPower &&
            version == vstPossibleDigimon[i].uiVersion)
            return &vstPossibleDigimon[i];
    }

    return NULL;
}

static Menu handleUserListRequest() {
    SDL_Rect clip = {0, 0, NORMAL_SIZE_SPRITE, NORMAL_SIZE_SPRITE};
    Menu result;

    int countUsers, i, j;
    unsigned char data[6];
    char path[100];

    if (SDLNet_TCP_Recv(connection, data, 6) != 6) {
        SDL_Log("Error receiving list of users");
        return result;
    }

    memcpy(&countUsers, data + 2, sizeof(countUsers));
    result.countOptions = countUsers;
    result.currentOption = 0;

    SDL_Log("%d digimons on server", countUsers);
    if (countUsers == 0) {
        SDL_Log("No one on server");
        return result;
    }

    result.options = calloc(countUsers, sizeof(Option));

    for (i = 0; i < countUsers && i < 50; i++) {
        SDLNet_TCP_Recv(connection, data, 6);

        digimon_t* currentDigimon = getDigimon(data[2], data[5]);
        if (currentDigimon == NULL) {
            SDL_Log("Digimon %d %d does not exist", data[2], data[5]);
            // TODO: Error handling
            continue;
        }

        snprintf(path, sizeof(path), "resource/%s.gif", currentDigimon->szName);
        for (j = 0; j < strlen(path); j++) {
            path[j] = tolower(path[j]);
        }

        addMenuImage(&result, path, clip);
    }

    return result;
}

int registerUser(digimon_t* digimon, Menu* menu) {
    if (connection == NULL || menu == NULL) {
        SDL_Log("Trying to register user without valid socket or menu");
        return 0;
    }

    TagLengthValue dataRegister[] = {
        {SLOT_POWER, sizeof(digimon->uiSlotPower), &digimon->uiSlotPower},
        {VERSION, sizeof(digimon->uiVersion), &digimon->uiVersion},
    };

    int size;
    unsigned char* data =
        allocBufferCOMM(BATTLE_REGISTER, dataRegister, 2, &size);
    if (data == NULL) {
        SDL_Log("Unable to prepare data to register user");
        return 0;
    }

    if (SDLNet_TCP_Send(connection, data, size) != size) {
        SDL_Log("Error sending registering data to server -> %s",
                SDLNet_GetError());
        free(data);
        return 0;
    }
    free(data);

    *menu = handleUserListRequest();
    return 1;
}

int getBattleList(Menu* menu) {
    if (connection == NULL || menu == NULL) {
        SDL_Log("Trying to register user without valid socket or menu");
        return 0;
    }

    unsigned char data = BATTLE_LIST;
    SDLNet_TCP_Send(connection, &data, sizeof(data));

    *menu = handleUserListRequest();
    return 1;
}

int disconnectFromServer() {
    SDLNet_TCP_Close(connection);
    connection = NULL;
    return 1;
}