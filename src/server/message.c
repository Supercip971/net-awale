#include "server/message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared/models/clientServer.h"

void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
    }
}
