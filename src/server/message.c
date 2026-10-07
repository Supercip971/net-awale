#include "server/message.h"

void write_client(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
    }
}

int parse_message(const char *buffer, ClientServerMessage *msg)
{
    char command[32] = {0};
    char arg[BUF_SIZE] = {0};

    int n = sscanf(buffer, "%31s %s", command, arg);
    if (n < 1)
        return -1;
    if (strcmp(command, "players") == 0)
        msg->kind = MSG_PLAYERS;
    else if (strcmp(command, "play") == 0 && n == 2)
    {
        msg->kind = MSG_PLAY;
        msg->play.hole = atoi(arg);
    }
    else
        return -1;
    return 0;
}