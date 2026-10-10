#pragma once
#include "shared/game.h"
#include "shared/models/const.h"

typedef enum
{
    MSG_CONNECT,
    MSG_PLAYERS,
    MSG_PLAY,
    MSG_DEFY,
    MSG_ACCEPT_DEFY,
    MSG_DECLINE_DEFY
} MsgType;

typedef enum
{
    RESULT_OK = 0,
    RESULT_ERROR
} ResultKind;

typedef char Pseudo[MAX_USERNAME_LENGTH];

typedef struct
{
    MsgType kind;

    ResultKind error; // non null if it contain an error
    union
    {
        // connect: empty
        struct
        {
            Pseudo pseudo[];
        } players;
        // played: empty
        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } accepted;
        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } declined;
        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } defyReceived;
        struct
        {
            Game game;
        } movePlayed;
    };
} ClientServerMessage;
