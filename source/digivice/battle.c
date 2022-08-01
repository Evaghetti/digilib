#include "digivice/battle.h"
#include "digibattle_classic.h"
#include "digivice/globals.h"
#include "digivice/texture.h"
#include "digiworld.h"
#include "enums.h"

#include <SDL_net.h>

#include <ctype.h>
#include <stdio.h>

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

static Player players[MAX_USER_COUNT], player;
static int countPlayers = 0, selectedPlayer = 0;
static State battleState = UPDATE_GAME;

static const Configuration* config;

static void toLowerStr(char* result) {
    while (*result != '\0') {
        *result = tolower(*result);
        result++;
    }
}

int connectToServer(digimon_t* playerDigimon) {
    config = getConfiguration();

    // Already connected
    if (connection != NULL)
        return 2;

    if (DIGIBATTLE_canBattle() != DIGIBATTLE_RET_OK) {
        SDL_Log("Digimon can't battle right now");
        return 0;
    }

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

    snprintf(player.pathSpriteSheet, sizeof(player.pathSpriteSheet),
             "resource/%s.gif", playerDigimon->szName);
    toLowerStr(player.pathSpriteSheet);
    player.slotPower = playerDigimon->uiSlotPower;
    player.version = playerDigimon->uiVersion;

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
    SDL_Rect clip = {0, 0, config->normalSpriteSize, config->normalSpriteSize};
    Menu result = {0};

    int i;
    unsigned char data[6];

    if (SDLNet_TCP_Recv(connection, data, 6) != 6) {
        SDL_Log("Error receiving list of users");
        return result;
    }

    memcpy(&countPlayers, data + 2, sizeof(countPlayers));

    SDL_Log("%d digimons on server", countPlayers);
    if (countPlayers == 0) {
        char* message[] = {"WAITING USERS"};
        result = initMenuText(1, message);
        result.customs = NO_CURSOR;
        return result;
    }

    result = initMenu(countPlayers, IMAGE);
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
        toLowerStr(players[i].pathSpriteSheet);
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
        char* paths[] = {"resource/popups.gif"};
        SDL_Rect clip = {0, config->normalSpriteSize * 4,
                         config->normalSpriteSize * 2,
                         config->normalSpriteSize};
        *menu = initMenuImage(1, paths, &clip);
        menu->customs = FILL_SCREEN | NO_CURSOR;
    }
    return dataReceived[2];
}

static SDL_Texture* generateHeaderChallenge(SDL_Renderer* renderer) {
    SDL_Color color = {0, 0, 0, 255};
    SDL_Texture* result = SDL_CreateTexture(
        renderer, SDL_PIXELFORMAT_RGBA32, SDL_TEXTUREACCESS_TARGET,
        config->widthScreen, config->heightSmallSprite);
    SDL_Texture *texturePlayer = loadTexture(player.pathSpriteSheet),
                *textureChallenger = getChallengedUserTexture();

    SDL_Rect transform = {config->widthSmallSprite / 2, 0,
                          config->widthSmallSprite, config->heightSmallSprite};
    SDL_Rect clip = {0, 0, config->normalSpriteSize, config->normalSpriteSize};

    SDL_SetRenderTarget(renderer, result);
    SDL_RenderClear(renderer);
    SDL_SetTextureBlendMode(result, SDL_BLENDMODE_BLEND);

    SDL_RenderCopyEx(renderer, textureChallenger, &clip, &transform, 0.f, NULL,
                     SDL_FLIP_HORIZONTAL);
    transform.x = config->widthScreen - transform.w * 1.5f;
    SDL_RenderCopy(renderer, texturePlayer, &clip, &transform);

    SDL_Texture* versusText = createTextTexture(color, "VS");
    transform.x = config->widthScreen * .45f;
    SDL_RenderCopy(renderer, versusText, NULL, &transform);

    SDL_SetRenderTarget(renderer, NULL);
    SDL_DestroyTexture(versusText);
    freeTexture(texturePlayer);
    freeTexture(textureChallenger);
    return result;
}

static void handleBattleRequest(Menu* menu, SDL_Renderer* renderer) {
    unsigned char dataReceived[3];
    SDLNet_TCP_Recv(connection, &dataReceived, sizeof(dataReceived));

    char uuidChallenger[SIZE_UUID] = {0};
    int i = 0;
    SDLNet_TCP_Recv(connection, dataReceived, 2);
    SDLNet_TCP_Recv(connection, uuidChallenger, sizeof(uuidChallenger));
    for (i = 0; i < countPlayers; i++) {
        SDL_Log("[ENZO] %s(%s) == %s", players[i].uuid,
                players[i].pathSpriteSheet, uuidChallenger);
        if (strcmp(players[i].uuid, uuidChallenger) == 0) {
            selectedPlayer = i;
            break;
        }
    }

    char* options[] = {"YES", "NO"};
    *menu = initMenuText(2, options);
    menu->header = generateHeaderChallenge(renderer);
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
    if (newMenu->countOptions) {
        memcpy(menu, newMenu, sizeof(Menu));
        menu->currentOption = originalIndex < menu->countOptions
                                  ? originalIndex
                                  : menu->countOptions - 1;
        if (menu->currentOption < 0)
            menu->currentOption = 0;
    }
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

StatusUpdate updateClient(Menu* menu, int selectedOption,
                          SDL_Renderer* renderer) {
    static unsigned char typeAction = BATTLE_LIST;
    StatusUpdate status = NOTHING_HAPPENED;
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

        DIGIBATTLE_changeStats(resultBattle);
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
            handleBattleRequest(&generatedMenu, renderer);
            if (response == ACCEPTED) {
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

static void initCachedDigimon(unsigned char minimumStage) {
    countPlayers = 0;
    int i;
    while (countPlayers < 5) {
        const int currentIndex = rand() % MAX_COUNT_DIGIMON;
        const digimon_t* currentDigimon = &vstPossibleDigimon[currentIndex];

        if (currentDigimon->uiStage < minimumStage)
            continue;
        else {
            for (i = 0; i < countPlayers; i++) {
                if (players[i].slotPower == currentDigimon->uiSlotPower &&
                    players[i].version == currentDigimon->uiVersion)
                    break;
            }

            if (i < countPlayers)
                continue;
        }

        players[countPlayers].slotPower = currentDigimon->uiSlotPower;
        players[countPlayers].version = currentDigimon->uiVersion;
        snprintf(players[countPlayers].pathSpriteSheet,
                 sizeof(players[countPlayers].pathSpriteSheet),
                 "resource/%s.gif", currentDigimon->szName);
        toLowerStr(players[countPlayers].pathSpriteSheet);
        countPlayers++;
    }
}

static void initMenuDigimon(Menu* menu) {
    const SDL_Rect clip = {0, 0, config->normalSpriteSize,
                           config->normalSpriteSize};
    int i;
    *menu = initMenu(countPlayers, IMAGE);
    for (i = 0; i < countPlayers; i++) {
        addMenuImage(menu, players[i].pathSpriteSheet, clip);
    }
}

static int localBattle(digimon_t* playerDigimon, int selectedOption) {

    unsigned char result = DIGIBATTLE_getBattleResult(
        playerDigimon->uiSlotPower, players[selectedOption].slotPower);
    DIGIBATTLE_changeStats(result);
    return result;
}

int updateSingleBattle(digimon_t* playerDigimon, Menu* menu,
                       int selectedOption) {
    config = getConfiguration();

    if (countPlayers == 0) {
        initCachedDigimon(playerDigimon->uiStage);
    }

    if (menu->countOptions == 0) {
        initMenuDigimon(menu);
        return -1;
    }

    switch (selectedOption) {
        case -2:
            freeMenu(menu);
            return 0;

        default:
            freeMenu(menu);
            selectedPlayer = selectedOption;
            return localBattle(playerDigimon, selectedOption);
        case -1:
        case -3:
            return -1;
    }
}

void resetPlayers() {
    memset(players, 0, sizeof(Player) * countPlayers);
    countPlayers = 0;
}

int cannBattle() {
    return DIGIBATTLE_canBattle() == DIGIBATTLE_RET_OK;
}