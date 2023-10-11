#include "wificom.h"

#include <json.h>
#include <mqtt.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "digibattle_classic.h"
#include "digihal.h"
#include "digivice/enums_digivice.h"

#if !defined(__POSIX_SOCKET_TEMPLATE_H__)
#define __POSIX_SOCKET_TEMPLATE_H__

#include <stdio.h>
#include <sys/types.h>
#if !defined(WIN32)
#include <netdb.h>
#include <sys/socket.h>
#include <unistd.h>
#else
#include <ws2tcpip.h>
#endif
#if defined(__VMS)
#include <ioctl.h>
#endif
#include <fcntl.h>
#include <pthread.h>

static int open_nb_socket(const char* addr, const char* port) {
    struct addrinfo hints = {0};

    hints.ai_family = AF_UNSPEC;     /* IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Must be TCP */
    int sockfd = -1;
    int rv;
    struct addrinfo *p, *servinfo;

    /* get address information */
    rv = getaddrinfo(addr, port, &hints, &servinfo);
    if (rv != 0) {
        LOG("Failed to open socket (getaddrinfo): %s", gai_strerror(rv));
        return -1;
    }

    /* open the first possible socket */
    for (p = servinfo; p != NULL; p = p->ai_next) {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd == -1)
            continue;

        /* connect to server */
        rv = connect(sockfd, p->ai_addr, p->ai_addrlen);
        if (rv == -1) {
            close(sockfd);
            sockfd = -1;
            continue;
        }
        break;
    }

    /* free servinfo */
    freeaddrinfo(servinfo);

    /* make non-blocking */
#if !defined(WIN32)
    if (sockfd != -1)
        fcntl(sockfd, F_SETFL, fcntl(sockfd, F_GETFL) | O_NONBLOCK);
#else
    if (sockfd != INVALID_SOCKET) {
        int iMode = 1;
        ioctlsocket(sockfd, FIONBIO, &iMode);
    }
#endif
#if defined(__VMS)
    /* 
        OpenVMS only partially implements fcntl. It works on file descriptors
        but silently fails on socket descriptors. So we need to fall back on
        to the older ioctl system to set non-blocking IO
    */
    int on = 1;
    if (sockfd != -1)
        ioctl(sockfd, FIONBIO, &on);
#endif

    /* return the new socket fd */
    return sockfd;
}

#endif

typedef struct battle_request_t {
    uint16_t* pviPackets;
    uint8_t iPacketCount;
    uint8_t iCurrentPacket;
    int iType;
    char szApplicationId[100];
    int iApiResponse;
    char szAckId[100];
    struct battle_request_t* next;
} battle_request_t;

typedef struct battle_result_t {
    uint16_t* pviPackets;
    uint8_t iPacketCount;
    uint8_t iCurrentPacket;
} battle_result_t;

#define WIFICOM_IP   "localhost"
#define WIFICOM_PORT "1883"

#define USERNAME    "admin"
#define PASSWORD    "b94eda2f-b7f1-437e-b9d6-5a178ed9e560"
#define USER_UUID   "1779137333937344"
#define DEVICE_UUID "1779141161573604"

#define INPUT_TOPIC  USERNAME "/f/" USER_UUID "-" DEVICE_UUID "/wificom-input"
#define OUTPUT_TOPIC USERNAME "/f/" USER_UUID "-" DEVICE_UUID "/wificom-output"

static struct mqtt_client gstClient;
static int giSockFd;
static uint8_t sendbuf[2048];
static uint8_t recvbuf[1024];
static battle_request_t stCurrentBattleRequest;
static battle_result_t stCurrentBattleResult;

static void* keepClientAlive(void* client) {
    while (1) {
        mqtt_sync((struct mqtt_client*)client);
        usleep(100000U);
    }
    return NULL;
}

static void parseDigirom(const char* pszDigiRom, battle_request_t* pstOut) {
    const char* pszCurrentToken = strstr(pszDigiRom, "-");
    pstOut->iPacketCount = pstOut->iPacketCount = 0;
    while (pszCurrentToken) {
        pstOut->iPacketCount++;
        pszCurrentToken = strstr(pszCurrentToken + 1, "-");
    }

    pstOut->pviPackets = calloc(pstOut->iPacketCount, sizeof(uint16_t));

    pszCurrentToken = strtok((char*)pszDigiRom, "-");
    pstOut->iType = strncmp("V1", pszCurrentToken, 2) == 0;

    int i = 0;
    pszCurrentToken = strtok(NULL, "-");
    while (pszCurrentToken != NULL) {
        pstOut->pviPackets[i] = strtol(pszCurrentToken, NULL, 16);
        i++;

        LOG("Packet 0x%04x", pstOut->pviPackets[i - 1]);
        pszCurrentToken = strtok(NULL, "-");
    }
}

static battle_request_t parseRequest(const char* pszPayload) {
    struct json_value_s* root = json_parse(pszPayload, strlen(pszPayload));

    battle_request_t stNewBattleRequest;
    memset(&stNewBattleRequest, 0, sizeof(stNewBattleRequest));

    if (root == NULL)
        return stNewBattleRequest;

    struct json_object_element_s* field =
        ((struct json_object_s*)root->payload)->start;

    while (field != NULL) {
        struct json_value_s* fieldValue = field->value;
        const char* pszFieldName = field->name->string;

        LOG("Campo %s", pszFieldName);
        if (strncmp(pszFieldName, "digirom", sizeof("digirom")) == 0) {
            const char* pszDigirom =
                ((struct json_string_s*)fieldValue->payload)->string;
            parseDigirom(pszDigirom, &stNewBattleRequest);
        }
        if (strncmp(pszFieldName, "application_id", sizeof("application_id")) ==
            0) {
            strncpy(stNewBattleRequest.szApplicationId,
                    ((struct json_string_s*)fieldValue->payload)->string,
                    sizeof(stNewBattleRequest.szApplicationId));
            LOG("É application_id! %s", stNewBattleRequest.szApplicationId);
        }
        if (strncmp(pszFieldName, "ack_id", sizeof("ack_id")) == 0) {
            strncpy(stNewBattleRequest.szAckId,
                    ((struct json_string_s*)fieldValue->payload)->string,
                    sizeof(stNewBattleRequest.szAckId));
            LOG("É ack_id! %s", stNewBattleRequest.szAckId);
        }
        if (strncmp(pszFieldName, "api_response", sizeof("api_response")) ==
            0) {
            stNewBattleRequest.iApiResponse = json_value_is_true(field->value);
            LOG("É api response! %d", stNewBattleRequest.iApiResponse);
        }

        field = field->next;
    }

    free(root);
    return stNewBattleRequest;
}

static void freeState() {
    LOG("Freeing battle state");
    free(stCurrentBattleRequest.pviPackets);
    free(stCurrentBattleResult.pviPackets);
    stCurrentBattleRequest.pviPackets = NULL;
    stCurrentBattleResult.pviPackets = NULL;

    memset(&stCurrentBattleRequest, 0, sizeof(stCurrentBattleRequest));
    memset(&stCurrentBattleResult, 0, sizeof(stCurrentBattleResult));
}

static void recvCallback(void** unused,
                         struct mqtt_response_publish* published) {
    char pszPayload[256] = {0};
    strncpy(pszPayload, (char*)published->application_message,
            sizeof(pszPayload) <= published->application_message_size
                ? sizeof(pszPayload)
                : published->application_message_size);
    LOG("Received battle payload %s", pszPayload);

    if (stCurrentBattleRequest.pviPackets != NULL) {
        freeState();
    }

    stCurrentBattleRequest = parseRequest(pszPayload);
    if (stCurrentBattleRequest.iPacketCount == 0)
        return;

    stCurrentBattleResult.iPacketCount =
        stCurrentBattleRequest.iPacketCount * 2;

    if (stCurrentBattleRequest.iPacketCount % 2 != 0)
        stCurrentBattleResult.iPacketCount++;

    stCurrentBattleResult.pviPackets =
        calloc(stCurrentBattleResult.iPacketCount, sizeof(uint16_t));
}

uint8_t WIFICOM_init() {
    giSockFd = open_nb_socket(WIFICOM_IP, WIFICOM_PORT);
    if (giSockFd == -1) {
        LOG("Error connecting to wificom");
        return DIGIVICE_RET_ERROR;
    }

    mqtt_init(&gstClient, giSockFd, sendbuf, sizeof(sendbuf), recvbuf,
              sizeof(recvbuf), recvCallback);
    mqtt_connect(&gstClient, USER_UUID, NULL, NULL, 0, "wificom", "wificom123",
                 MQTT_CONNECT_CLEAN_SESSION | MQTT_CONNECT_USER_NAME |
                     MQTT_CONNECT_PASSWORD,
                 400);
    if (gstClient.error != MQTT_OK) {
        LOG("Error connecting mqtt -> %s", mqtt_error_str(gstClient.error));
        return DIGIVICE_RET_ERROR;
    }

    pthread_t stClientDaemon;
    if (pthread_create(&stClientDaemon, NULL, keepClientAlive, &gstClient)) {
        LOG("Failed to start client daemon.");
        return DIGIVICE_RET_ERROR;
    }

    mqtt_subscribe(&gstClient, INPUT_TOPIC, 0);
    LOG("Subscribing to %s", INPUT_TOPIC);
    if (gstClient.error != MQTT_OK) {
        pthread_cancel(stClientDaemon);
        LOG("Error subscribing -> %s", mqtt_error_str(gstClient.error));
        return DIGIVICE_RET_ERROR;
    }

    return DIGIVICE_RET_OK;
}

char* generatePayloadResponse(const battle_result_t* pstCurrentBattleResult,
                              const battle_request_t* pstCurrentBattleRequest) {
    const size_t uiMaxSize = pstCurrentBattleResult->iPacketCount * 7;
    char* pszResultBattle = calloc(uiMaxSize, sizeof(char));

    size_t uiOffset = 0;
    int i, iType = pstCurrentBattleRequest->iType;
    for (i = 0; i < pstCurrentBattleResult->iPacketCount; i++) {
        if (iType) {
            uiOffset +=
                snprintf(pszResultBattle + uiOffset, uiMaxSize - uiOffset,
                         "s:%04x ", pstCurrentBattleResult->pviPackets[i]);
        } else {
            uiOffset +=
                snprintf(pszResultBattle + uiOffset, uiMaxSize - uiOffset,
                         "r:%04x ", pstCurrentBattleResult->pviPackets[i]);
        }

        iType = !iType;
    }

    size_t uiResultSize = strlen(pszResultBattle) + 200;
    char* pszResult = calloc(uiResultSize, sizeof(char));
    snprintf(pszResult, uiResultSize,
             "{\"application_uuid\":%s, \"device_uuid\":\"%s\",\"output\":"
             "\"%s\",\"ack_id\":\"%s\"}",
             pstCurrentBattleRequest->szApplicationId, DEVICE_UUID,
             pszResultBattle, pstCurrentBattleRequest->szAckId);

    free(pszResultBattle);
    return pszResult;
}

static uint8_t handleFinalPacket() {
    if (stCurrentBattleResult.iCurrentPacket >=
        stCurrentBattleResult.iPacketCount) {

        char* pszPayloadResponse = generatePayloadResponse(
            &stCurrentBattleResult, &stCurrentBattleRequest);

        mqtt_publish(&gstClient, OUTPUT_TOPIC, pszPayloadResponse,
                     strlen(pszPayloadResponse), MQTT_PUBLISH_QOS_0);
        LOG("Result being sent: %s", pszPayloadResponse);
        free(pszPayloadResponse);
        freeState();

        if (gstClient.error != MQTT_OK) {
            LOG("Error sending result: %s", mqtt_error_str(gstClient.error));
            return DIGIBATTLE_RET_ERROR;
        }
    }

    return DIGIBATTLE_RET_OK;
}

uint16_t WIFICOM_send(uint16_t uiPacket) {
    if (stCurrentBattleResult.iPacketCount == 0)
        return DIGIBATTLE_RET_ERROR;
    stCurrentBattleResult.pviPackets[stCurrentBattleResult.iCurrentPacket] =
        uiPacket;
    stCurrentBattleResult.iCurrentPacket++;

    uint8_t uiRetHandle = handleFinalPacket();
    if (uiRetHandle)
        return uiRetHandle;

    LOG("Current packet send: %04x", uiPacket);
    return DIGIBATTLE_RET_OK;
}

uint16_t WIFICOM_recv() {
    if (stCurrentBattleRequest.pviPackets == NULL ||
        (stCurrentBattleResult.iCurrentPacket == 0 &&
         stCurrentBattleRequest.iType == 0))
        return DIGIBATTLE_RET_POLL;

    if (stCurrentBattleRequest.iPacketCount &&
        stCurrentBattleRequest.iCurrentPacket >=
            stCurrentBattleRequest.iPacketCount) {
        freeState();
        return DIGIBATTLE_RET_ERROR;
    }

    uint16_t uiCurrentPacket =
        stCurrentBattleRequest
            .pviPackets[stCurrentBattleRequest.iCurrentPacket];
    stCurrentBattleResult.pviPackets[stCurrentBattleResult.iCurrentPacket] =
        uiCurrentPacket;

    stCurrentBattleRequest.iCurrentPacket++;
    stCurrentBattleResult.iCurrentPacket++;

    LOG("Current packet recv: %04x", uiCurrentPacket);
    return uiCurrentPacket;
}
