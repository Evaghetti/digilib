#include "digivice/battle.h"
#include "digibattle_classic.h"
#include "digivice/globals.h"
#include "digiworld.h"

#include "SDL2/SDL_net.h"

#define IP   "localhost"
#define PORT 1998

#define SIZE_UUID      36
#define MAX_USER_COUNT 50

#define TLV_LENGTH(x) (sizeof(x) / sizeof(x[0]))

// States
typedef enum States {
    BATTLE_REGISTER,
    BATTLE_CHALLENGE,
    BATTLE_REQUEST,
    BATTLE_LIST,
    UPDATE_GAME,
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

static char playersUUIDs[MAX_USER_COUNT][SIZE_UUID + 1];

int connectToServer() {
    // Already connected
    if (connection != NULL)
        return 2;

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

static int sendData(State state, TagLengthValue* tags, int tagsLength) {
    int sizeData;
    unsigned char* data = allocBufferCOMM(state, tags, tagsLength, &sizeData);
    if (data == NULL) {
        SDL_Log("Unable to alloc memory send to server");
        return 0;
    }

    if (SDLNet_TCP_Send(connection, data, sizeData) != sizeData) {
        SDL_Log("Error sending data to server -> %s", SDLNet_GetError());
        free(data);
        return 0;
    }
    free(data);
    return 1;
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
    Menu result = {0};

    int countUsers, i, j;
    unsigned char data[6];
    char path[100];

    if (SDLNet_TCP_Recv(connection, data, 6) != 6) {
        SDL_Log("Error receiving list of users");
        return result;
    }

    memcpy(&countUsers, data + 2, sizeof(countUsers));
    result.countOptions = countUsers;
    result.type = IMAGE;
    result.currentOption = 0;

    SDL_Log("%d digimons on server", countUsers);
    if (countUsers == 0) {
        SDL_Log("No one on server");
        return result;
    }

    result.options = calloc(countUsers, sizeof(Option));

    for (i = 0; i < countUsers && i < MAX_USER_COUNT; i++) {
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

        SDLNet_TCP_Recv(connection, data, 2);
        SDLNet_TCP_Recv(connection, playersUUIDs[i], data[1]);
        SDL_Log("User uuid -> %s", playersUUIDs[i]);
    }

    return result;
}

int registerUser(digimon_t* digimon) {
    if (connection == NULL) {
        SDL_Log("Trying to register user without valid socket");
        return 0;
    }

    TagLengthValue dataRegister[] = {
        {SLOT_POWER, sizeof(digimon->uiSlotPower), &digimon->uiSlotPower},
        {VERSION, sizeof(digimon->uiVersion), &digimon->uiVersion},
    };

    if (!sendData(BATTLE_REGISTER, dataRegister, TLV_LENGTH(dataRegister))) {
        SDL_Log("Unable to register");
        return 0;
    }

    unsigned char byte;
    SDLNet_TCP_Recv(connection, &byte, 1);
    return 1;
}

static void changeMenuKeepIndex(Menu* menu, Menu* newMenu) {
    const int originalIndex = menu->currentOption;

    freeMenu(menu);
    memcpy(menu, newMenu, sizeof(Menu));
    menu->currentOption = originalIndex < menu->countOptions
                              ? originalIndex
                              : menu->countOptions - 1;
    if (menu->currentOption < 0)
        menu->currentOption = 0;
}

int updateClient(Menu* menu) {
    Menu generatedMenu;
    if (connection == NULL || menu == NULL) {
        SDL_Log("Trying to register user without valid socket or menu");
        return 0;
    }

    // Say to server i want an update, then get the type of update
    unsigned char typeAction = UPDATE_GAME;
    SDLNet_TCP_Send(connection, &typeAction, sizeof(typeAction));
    SDLNet_TCP_Recv(connection, &typeAction, sizeof(typeAction));

    switch (typeAction) {
        case BATTLE_LIST:
            generatedMenu = handleUserListRequest();
            break;
    }

    changeMenuKeepIndex(menu, &generatedMenu);
    return 1;
}

int disconnectFromServer() {
    SDLNet_TCP_Close(connection);
    connection = NULL;
    return 1;
}