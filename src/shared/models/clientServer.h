#pragma once
#include "shared/models/const.h"

typedef enum
{
    MSG_CONNECT,
    MSG_PLAYERS,
    MSG_GAMES,
    MSG_PLAY,
    MSG_DEFY,
    MSG_ACCEPT_DEFY,
    MSG_DECLINE_DEFY,
    MSG_MESSAGE,
    MSG_SETBIO
} MsgType;

typedef struct
{
    MsgType kind;
    union
    {
        struct
        {
        } players;
        struct
        {
        } games;
        struct
        {
            int hole;
        } play;
        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } defy;
        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } acceptDefy;
        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } declineDefy;
        struct
        {
            char message[BUF_SIZE - 9];
        } message;
        struct
        {
            char bio[BUF_SIZE - 7];
        } setbio;
    };
} ClientServerMessage;