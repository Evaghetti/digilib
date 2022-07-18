#include "digivice/battle.h"
#include "digibattle_classic.h"
#include "digivice/globals.h"
#include "digivice/texture.h"
#include "digiworld.h"

#include "SDL2/SDL_net.h"

#define IP   "localhost"
#define PORT 1998

#define SIZE_UUID      36
#define SIZE_PATH      100
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
    USER_ID,
    RESPONSE,
} Tag;

typedef enum ResponsesRequest {
    NO_RESPONSE,
    ACCEPTED,
    REFUSED
} ResponseRequest;

typedef struct {
    unsigned char tag;
    unsigned char length;
    void* value;
} TagLengthValue;

typedef struct {
    unsigned char slotPower;
    unsigned char version;
    char pathSpriteSheet[SIZE_PATH + 1];
    char uuid[SIZE_UUID + 1];
} Player;

static TCPsocket connection = NULL;

static Player players[MAX_USER_COUNT];
static int countPlayers = 0, selectedPlayer = 0;
static State battleState = UPDATE_GAME;

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

static digimon_t* getDigimon(unsigned char slot, unsigned char version) {
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

    int i, j;
    unsigned char data[6];
    char path[100];

    if (SDLNet_TCP_Recv(connection, data, 6) != 6) {
        SDL_Log("Error receiving list of users");
        return result;
    }

    memcpy(&countPlayers, data + 2, sizeof(countPlayers));
    result.countOptions = countPlayers;
    result.type = IMAGE;
    result.currentOption = 0;

    SDL_Log("%d digimons on server", countPlayers);
    if (countPlayers == 0) {
        SDL_Log("No one on server");
        return result;
    }

    result.options = calloc(countPlayers, sizeof(Option));

    for (i = 0; i < countPlayers && i < MAX_USER_COUNT; i++) {
        SDLNet_TCP_Recv(connection, data, 6);

        players[i].slotPower = data[2];
        players[i].version = data[5];

        digimon_t* currentDigimon =
            getDigimon(players[i].slotPower, players[i].version);
        if (currentDigimon == NULL) {
            SDL_Log("Digimon %d %d does not exist", players[i].slotPower,
                    players[i].version);
            countPlayers--;
            // TODO: Error handling
            continue;
        }

        snprintf(players[i].pathSpriteSheet, sizeof(players[i].pathSpriteSheet),
                 "resource/%s.gif", currentDigimon->szName);
        for (j = 0; j < strlen(players[i].pathSpriteSheet); j++) {
            players[i].pathSpriteSheet[j] =
                tolower(players[i].pathSpriteSheet[j]);
        }

        addMenuImage(&result, players[i].pathSpriteSheet, clip);

        SDLNet_TCP_Recv(connection, data, 2);
        SDLNet_TCP_Recv(connection, players[i].uuid, data[1]);
        SDL_Log("User uuid -> %s", players[i].uuid);
    }

    return result;
}

static int handleBattleChallenge(Menu* menu) {
    unsigned char dataReceived[3];
    SDLNet_TCP_Recv(connection, &dataReceived, sizeof(dataReceived));

    SDL_Log("Response from challenged -> %d", dataReceived[2]);
    if (dataReceived[2] == 0) {
        char* options[] = {"BATTLE"};
        *menu = initMenuText(1, options);
    }
    return dataReceived[2];
}

static void handleBattleRequest(Menu* menu) {
    unsigned char dataReceived[3];
    SDLNet_TCP_Recv(connection, &dataReceived, sizeof(dataReceived));

    char* options[] = {"YES", "NO"};
    *menu = initMenuText(2, options);
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

static uint16_t recvImplementation() {
    uint16_t dataReceived;
    SDLNet_TCP_Recv(connection, &dataReceived, sizeof(dataReceived));
    return dataReceived;
}

static uint16_t sendImplementation(uint16_t data) {
    SDLNet_TCP_Send(connection, &data, sizeof(data));
    return 0;
}

StatusUpdate updateClient(Menu* menu, int selectedOption) {
    static unsigned char typeAction = BATTLE_LIST;
    StatusUpdate status = NOTHING_HAPPENED;
    char* options[2];
    Menu generatedMenu;
    if (connection == NULL || menu == NULL) {
        SDL_Log("Trying to register user without valid socket or menu");
        return ERROR;
    }

    if (battleState != UPDATE_GAME) {
        uint8_t resultBattle;
        if (battleState == BATTLE_CHALLENGE) {
            resultBattle =
                DIGIBATTLE_initiate(&sendImplementation, &recvImplementation);
        } else {
            resultBattle =
                DIGIBATTLE_continue(&sendImplementation, &recvImplementation);
        }
        battleState = UPDATE_GAME;
        return (StatusUpdate)resultBattle;
    }

    // Say to server i want an update, then get the type of update
    unsigned char typeData = UPDATE_GAME, response = NO_RESPONSE;
    if ((selectedOption != -1 && selectedOption != -3) &&
        typeAction == BATTLE_REQUEST) {
        if (selectedOption == -2 || selectedOption == 1)
            response = REFUSED;
        else
            response = ACCEPTED;
        status = HANDLING_MENU;
    }

    TagLengthValue dataUpdateRequest[] = {
        {RESPONSE, sizeof(response), &response}};
    sendData(typeData, dataUpdateRequest, TLV_LENGTH(dataUpdateRequest));
    SDLNet_TCP_Recv(connection, &typeAction, sizeof(typeAction));

    SDL_Log("Type of Online action -> %d", typeAction);
    switch (typeAction) {
        case BATTLE_LIST:
            generatedMenu = handleUserListRequest();
            break;
        case BATTLE_CHALLENGE:
            if (handleBattleChallenge(&generatedMenu) == 1) {
                battleState = BATTLE_CHALLENGE;
            }
            break;
        case BATTLE_REQUEST:
            handleBattleRequest(&generatedMenu);
            if (selectedOption == 0) {
                battleState = BATTLE_REQUEST;
            }
            break;
        default:
            break;
    }

    changeMenuKeepIndex(menu, &generatedMenu);
    return status;
}

int challengeUser(int offsetUser) {
    if (connection == NULL) {
        SDL_Log("Trying to challenge user without valid connection");
        return 0;
    }
    if (offsetUser < 0 && offsetUser >= countPlayers) {
        SDL_Log("User %d does not exist", offsetUser);
        return 0;
    }

    TagLengthValue dataSent[] = {
        {USER_ID, strlen(players[offsetUser].uuid), players[offsetUser].uuid},
    };
    sendData(BATTLE_CHALLENGE, dataSent, TLV_LENGTH(dataSent));
    selectedPlayer = offsetUser;

    unsigned char status;
    SDLNet_TCP_Recv(connection, &status, sizeof(status));

    return status;
}

SDL_Texture* getChallengedUserTexture() {
    if (selectedPlayer < 0 || selectedPlayer >= countPlayers)
        return NULL;

    return loadTexture(players[selectedPlayer].pathSpriteSheet);
}

int disconnectFromServer() {
    SDLNet_TCP_Close(connection);
    connection = NULL;
    return 1;
}