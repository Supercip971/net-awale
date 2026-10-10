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

    if (sscanf(buffer, "%31s %s", command, arg) < 1)
        return -1;

    if (strcmp(command, "players") == 0)
    {
        msg->kind = MSG_PLAYERS;
    }
    else if (strcmp(command, "games") == 0)
    {
        msg->kind = MSG_GAMES;
    }
    else if (strcmp(command, "play") == 0)
    {
        msg->kind = MSG_PLAY;
        msg->play.hole = atoi(arg);
    }
    else if (strcmp(command, "message") == 0)
    {
        const char *space = strchr(buffer, ' ');
        if (space == NULL || space[1] == 0)
            return -1;
        msg->kind = MSG_MESSAGE;
        strncpy(msg->message.message, space + 1, sizeof msg->message.message - 1);
        msg->message.message[sizeof msg->message.message - 1] = 0;
    }
    else if (strcmp(command, "defy") == 0)
    {
        if (arg[0] == 0)
            return -1;

        msg->kind = MSG_DEFY;
        strncpy(msg->defy.pseudo, arg, sizeof msg->defy.pseudo - 1);
        msg->defy.pseudo[sizeof msg->defy.pseudo - 1] = 0;
    }
    else if (strcmp(command, "decline") == 0)
    {
        if (arg[0] == 0)
            return -1;

        msg->kind = MSG_DECLINE_DEFY;
        strncpy(msg->declineDefy.pseudo, arg, sizeof msg->declineDefy.pseudo - 1);
        msg->declineDefy.pseudo[sizeof msg->declineDefy.pseudo - 1] = 0;
    }
    else if (strcmp(command, "accept") == 0)
    {
        if (arg[0] == 0)
            return -1;

        msg->kind = MSG_ACCEPT_DEFY;
        strncpy(msg->acceptDefy.pseudo, arg, sizeof msg->acceptDefy.pseudo - 1);
        msg->acceptDefy.pseudo[sizeof msg->acceptDefy.pseudo - 1] = 0;
    }
    else if (strcmp(command, "spec") == 0)
    {
        if (arg[0] == 0)
            return -1;

        msg->kind = MSG_SPEC;
        strncpy(msg->spec.pseudo, arg, sizeof msg->spec.pseudo - 1);
        msg->spec.pseudo[sizeof msg->spec.pseudo - 1] = 0;
    }
    else if (strcmp(command, "stopspec") == 0)
    {
        msg->kind = MSG_STOP_SPEC;
    }
    else if (strcmp(command, "setbio") == 0)
    {
        const char *space = strchr(buffer, ' ');
        if (space == NULL || space[1] == 0)
            return -1;
        msg->kind = MSG_SETBIO;
        strncpy(msg->setbio.bio, space + 1, sizeof msg->setbio.bio - 1);
        msg->setbio.bio[sizeof msg->setbio.bio - 1] = 0;
    }
    else if (strcmp(command, "info") == 0)
    {
        msg->kind = MSG_INFO;
        strncpy(msg->info.pseudo, arg, sizeof msg->info.pseudo - 1);
        msg->info.pseudo[sizeof msg->info.pseudo - 1] = 0;
    }
    else
    {

        fprintf(stderr, "Unknown command: %s\n", command);
        return -1;
    }
    return 0;
}
