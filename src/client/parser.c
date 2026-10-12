
#include "parser.h"
#include <stdio.h>
#include "shared/models/clientServer.h"

ClientServerMessage *encode_message(const char *buffer)
{
    ClientServerMessage *msg = malloc(sizeof(ClientServerMessage));

    char command[32] = {0};
    char arg[BUF_SIZE] = {0};

    *msg = (ClientServerMessage){0};
    msg->protocol_magic = PROTOCOL_MAGIC;

    if (sscanf(buffer, "%31s %s", command, arg) < 1)
        return NULL;

    if (strcmp(command, "help") == 0)
    {
        msg->kind = MSG_CLIENTSIDE_HELP;
    }
    else if (strcmp(command, "players") == 0)
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
            return NULL;
        msg->kind = MSG_MESSAGE;
        strncpy(msg->message.message, space + 1, sizeof msg->message.message - 1);
        msg->message.message[sizeof msg->message.message - 1] = 0;
    }
    else if (strcmp(command, "defy") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_DEFY;
        strncpy(msg->defy.pseudo, arg, sizeof msg->defy.pseudo - 1);
        msg->defy.pseudo[sizeof msg->defy.pseudo - 1] = 0;
    }
    else if (strcmp(command, "decline") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_DECLINE_DEFY;
        strncpy(msg->declineDefy.pseudo, arg, sizeof msg->declineDefy.pseudo - 1);
        msg->declineDefy.pseudo[sizeof msg->declineDefy.pseudo - 1] = 0;
    }
    else if (strcmp(command, "accept") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_ACCEPT_DEFY;
        strncpy(msg->acceptDefy.pseudo, arg, sizeof msg->acceptDefy.pseudo - 1);
        msg->acceptDefy.pseudo[sizeof msg->acceptDefy.pseudo - 1] = 0;
    }
    else if (strcmp(command, "spec") == 0)
    {
        if (arg[0] == 0)
            return NULL;

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
            return NULL;
        msg->kind = MSG_SETBIO;
        strncpy(msg->setbio.bio, space + 1, sizeof msg->setbio.bio - 1);
        msg->setbio.bio[sizeof msg->setbio.bio - 1] = 0;
    }
    else if (strcmp(command, "info") == 0)
    {
        msg->kind = MSG_INFO;
        strncpy(msg->info.pseudo, arg, sizeof msg->info.pseudo - 1);
    }
    else if (strcmp(command, "setprivate") == 0)
    {
        msg->kind = MSG_SET_PRIVATE;
        msg->setPrivate.private = atoi(arg);
    }
    else if (strcmp(command, "friends") == 0)
    {
        msg->kind = MSG_FRIENDS;
    }
    else if (strcmp(command, "addfriend") == 0)
    {
        msg->kind = MSG_ADDFRIEND;
        if (arg[0] == 0)
            return NULL;

        strncpy(msg->addFriend.pseudo, arg, sizeof msg->addFriend.pseudo - 1);
    }
    else if (strcmp(command, "history") == 0)
    {
        msg->kind = MSG_HISTORY;
    }
    else if (strcmp(command, "replaygame") == 0)
    {
        msg->kind = MSG_REPLAY_GAME;
        msg->replayGame.id = atoi(arg);
    }
    else if (strcmp(command, "s") == 0)
    {
        msg->kind = MSG_STEP_REPLAY;
    }
    else if (strcmp(command, "c") == 0)
    {
        msg->kind = MSG_CONTINUE_REPLAY;
    }
    else
    {
        return NULL;
    }
    return msg;
}
