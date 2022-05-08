#include "digicomm.h"

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <string.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <unistd.h>

#define ADDRESS     "127.0.0.1"
#define SERVER_PORT 1997

static int gSocketServer = 0, gSocketClient = 0;
static struct sockaddr_in gServerconfig;

int createSocket() {
    // Cria o socket.
    int createdSocket = socket(AF_INET, SOCK_STREAM, 0), opt = 1;
    // Caso tenha algum problema ao criar o socket;
    if (createdSocket == -1)
        return 0;
    // Configura o socket para que ele possa reaproveitar o endereço e porta.
    int result =
        setsockopt(createdSocket, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    if (result != 0)
        return 0;

    struct timeval tv;
    tv.tv_sec = 5;
    tv.tv_usec = 0;
    setsockopt(createdSocket, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv,
               sizeof(tv));

    return createdSocket;
}

void connectTo(int socketDescriptor) {
    struct sockaddr_in config;

    // Configura pra fazer a conexão.
    config.sin_family = AF_INET;
    config.sin_addr.s_addr = inet_addr(ADDRESS);
    config.sin_port = htons(SERVER_PORT);

    printf("Tryng to connect to %s on port %d...\n", ADDRESS, SERVER_PORT);

    int result =
        connect(socketDescriptor, (struct sockaddr*)&config, sizeof(config));
    if (result != 0) {
        fprintf(stderr, "Erro %d:%s\n", errno, strerror(errno));
        return;
    }

    printf("Connected\n");
}

void* serverLogic() {
    int sockets[2] = {0, 0}, i, maxSocket;
    uint16_t uiPacket;
    fd_set readfds;

    while (1) {
        int hasEmpty = 0;
        FD_ZERO(&readfds);

        FD_SET(gSocketServer, &readfds);
        maxSocket = gSocketServer;

        for (i = 0; i < 2; i++) {
            if (sockets[i] == 0) {
                hasEmpty = 1;
                continue;
            }

            if (sockets[i] > 0)
                FD_SET(sockets[i], &readfds);
            if (sockets[i] > maxSocket)
                maxSocket = sockets[i];
        }

        select(maxSocket + 1, &readfds, NULL, NULL, NULL);

        if (FD_ISSET(gSocketServer, &readfds)) {
            printf("Conectaram\n");
            for (i = 0; i < 2; i++) {
                if (sockets[i] == 0)
                    break;
            }

            int sizeConfig = sizeof(gServerconfig);
            sockets[i] = accept(gSocketServer, (struct sockaddr*)&gServerconfig,
                                &sizeConfig);
        }

        if (hasEmpty)
            continue;

        for (i = 0; i < 2; i++) {
            int currentSocket = sockets[i], nextSocket = sockets[(i + 1) % 2];
            if (FD_ISSET(currentSocket, &readfds)) {
                if (recv(currentSocket, &uiPacket, sizeof(uiPacket),
                         MSG_DONTWAIT) == 0) {
                    printf("Fechou\n");
                    close(currentSocket);
                    sockets[i] = 0;
                } else if (nextSocket) {
                    printf("Enviando pro próximo %04x\n", uiPacket);
                    write(nextSocket, &uiPacket, sizeof(uiPacket));
                }
            }
        }
    }
    return NULL;
}

uint16_t DIGICOMM_setup() {
    if (gSocketServer == 0) {
        gSocketServer = createSocket();

        // Configura para esperar uma conexão no localhost porta 1337
        gServerconfig.sin_family = AF_INET;
        gServerconfig.sin_addr.s_addr = INADDR_ANY;
        gServerconfig.sin_port = htons(SERVER_PORT);

        // Coloca o socket oficialmente nas configurações acima.
        int result = bind(gSocketServer, (struct sockaddr*)&gServerconfig,
                          sizeof(gServerconfig));
        printf("Listening on address %s:%d\n", ADDRESS, SERVER_PORT);
        result =
            listen(gSocketServer, 2);  // Aceita apenas uma conexão por vez.
        if (result != 0)
            return DIGICOMM_ERROR_POLLING;

        pthread_t ulThreadServer;
        pthread_create(&ulThreadServer, NULL, &serverLogic, NULL);
    }

    gSocketClient = createSocket();
    connectTo(gSocketClient);
    return DIGICOMM_OK;
}

uint16_t DIGICOMM_pollData() {
    uint16_t uiData;

    int iRet = read(gSocketClient, &uiData, sizeof(uiData));
    if (iRet == EAGAIN || iRet == EWOULDBLOCK)
        return DIGICOMM_ERROR_POLLING;
    else if (iRet != sizeof(uiData))
        return DIGICOMM_ERROR_READING;

    return uiData;
}

uint16_t DIGICOMM_sendData(uint16_t uiData) {
    if (write(gSocketClient, &uiData, sizeof(uiData)) != sizeof(uiData))
        return DIGICOMM_ERROR_WRITING;
    return DIGICOMM_OK;
}

void DIGICOMM_close() {
    close(gSocketClient);
    gSocketClient = 0;
}
