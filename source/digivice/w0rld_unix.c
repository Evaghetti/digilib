#include "digivice/globals.h"
#include "digivice/w0rld.h"

#include <SDL.h>

#include <arpa/inet.h>  //inet_addr
#include <fcntl.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static struct timeval timeout;
static int fd = -1;

static int createFd() {
    int result = -1;
#ifndef _ANDROID_BUILD_
    result = open("/dev/pts/5", O_RDWR | O_APPEND | O_NONBLOCK);
#else
    struct sockaddr_in server;

    //Create socket
    result = socket(AF_INET, SOCK_STREAM, 0);
    if (result == -1) {
        SDL_Log("Could not create socket");
        return result;
    }

    server.sin_addr.s_addr = inet_addr("192.168.100.65");
    server.sin_family = AF_INET;
    server.sin_port = htons(1997);

    //Connect to remote server
    if (connect(result, (struct sockaddr*)&server, sizeof(server)) < 0) {
        SDL_Log("Connection error for w0rld");
        close(result);
        return -1;
    }
#endif

    timeout.tv_sec = 1;
    SDL_Log("Created %d", result);
    return result;
}

int prepareDCOMLogic() {
    if (fd != -1)
        return 1;

    fd = createFd();
    if (fd == -1) {
        SDL_Log("Error preparing DCOM");
        return 0;
    }

    SDL_Log("DCOM prepared %d", fd);
    return 1;
}

void releaseDCOMLogic() {
    if (fd == -1)
        return;

    SDL_Log("DCOM released");
    close(fd);

    fd = -1;
}

int readDataDCOM(void* dst, int sizeDst) {
#ifdef _ANDROID_BUILD_
    fd_set set;

    FD_ZERO(&set);
    FD_SET(fd, &set);

    switch (select(1, &set, NULL, NULL, &timeout)) {
        case -1:
            return -1;
        case 0:
            return 0;
        default:
            return read(fd, dst, sizeDst) > 0;
    }
#endif

    return read(fd, dst, sizeDst) > 0;
}

int writeDataDCOM(const void* dst, int sizeDst) {
    return write(fd, dst, sizeDst) != -1;
}