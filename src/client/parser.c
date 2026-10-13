
#include "parser.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "shared/helper.h"
#include "shared/models/clientServer.h"

ClientServerMessage *encode_message(const char *buffer)
{
    ClientServerMessage *msg = malloc(sizeof(ClientServerMessage));

    char command[32] = {0};
    char arg[BUF_SIZE] = {0};

    *msg = (ClientServerMessage){0};
    msg->protocol_magic = PROTOCOL_MAGIC;

    if (sscanf(buffer, "%31s %991s", command, arg) < 1)
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
        strccpy(msg->message.message, space + 1, sizeof(msg->message.message));
    }
    else if (strcmp(command, "defy") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_DEFY;
        strccpy(msg->defy.pseudo, arg, sizeof(msg->defy.pseudo));
    }
    else if (strcmp(command, "decline") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_DECLINE_DEFY;
        strccpy(msg->declineDefy.pseudo, arg, sizeof(msg->declineDefy.pseudo));
    }
    else if (strcmp(command, "accept") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_ACCEPT_DEFY;
        strccpy(msg->acceptDefy.pseudo, arg, sizeof(msg->acceptDefy.pseudo));
    }
    else if (strcmp(command, "cancel") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_CANCEL_DEFY;
        strccpy(msg->cancelDefy.pseudo, arg, sizeof(msg->cancelDefy.pseudo));
    }
    else if (strcmp(command, "spec") == 0)
    {
        if (arg[0] == 0)
            return NULL;

        msg->kind = MSG_SPEC;
        strccpy(msg->spec.pseudo, arg, sizeof(msg->spec.pseudo));
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
        strccpy(msg->setbio.bio, space + 1, sizeof(msg->setbio.bio));
    }
    else if (strcmp(command, "info") == 0)
    {
        msg->kind = MSG_INFO;
        strccpy(msg->info.pseudo, arg, sizeof(msg->info.pseudo));
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

        strccpy(msg->addFriend.pseudo, arg, sizeof(msg->addFriend.pseudo));
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
    else if (strcmp(command, "giveup") == 0)
    {
        msg->kind = MSG_GIVE_UP;
    }
    else
    {
        return NULL;
    }
    return msg;
}
