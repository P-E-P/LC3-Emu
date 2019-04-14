#include <sys/select.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include "kbd.h"

uint16_t check_key()
{
    fd_set rfd;
    FD_ZERO(&rfd);
    FD_SET(STDIN_FILENO, &rfd);

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;

    return select(1, &rfd, NULL, NULL, &timeout) != 0; 
}
