#pragma once
#include "shared/common.h"
#include "shared/models/const.h"

typedef enum
{
    MSG_STATUS,
    MSG_CLIENTSIDE_HELP,
    MSG_CONNECT,
    MSG_PLAYERS,
    MSG_INFO,
    MSG_GAMES,
    MSG_PLAY,
    MSG_DEFY,
    MSG_ACCEPT_DEFY,
    MSG_DECLINE_DEFY,
    MSG_CANCEL_DEFY,
    MSG_MESSAGE,
    MSG_SPEC,
    MSG_STOP_SPEC,
    MSG_SETBIO,
    MSG_SET_PRIVATE,
    MSG_FRIENDS,
    MSG_ADDFRIEND,
    MSG_HISTORY,
    MSG_REPLAY_GAME,
    MSG_STEP_REPLAY,
    MSG_GIVE_UP,
    MSG_CONTINUE_REPLAY
} MsgType;

typedef struct
{
    uint64_t protocol_magic;
    MsgType kind;
    union
    {
        struct
        {
            bool is_error;
            char msg[BUF_SIZE - 32];
        } status;
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
            char pseudo[MAX_USERNAME_LENGTH];
        } cancelDefy;

        struct
        {
            char message[BUF_SIZE - 9];
        } message;

        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } spec;

        struct
        {
        } stopSpec;

        struct
        {
            char bio[BUF_SIZE - 7];
        } setbio;

        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } info;

        struct
        {
            int private;
        } setPrivate;

        struct
        {
            char pseudo[MAX_USERNAME_LENGTH];
        } addFriend;

        struct
        {
        } friend;

        struct
        {
        } history;

        struct
        {
            GameHistoryId id;
        } replayGame;

        struct
        {
        } nextReplay;

        struct
        {
        } continueReplay;
    };
} ClientServerMessage;

#define CALL_SERVER_LEN sizeof(ClientServerMessage)
