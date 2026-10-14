#pragma once
#include "shared/common.h"
#include "shared/game.h"
#include "shared/models/const.h"
#include "shared/models/shared.h"

typedef enum
{
    MSG_RET_STATUS, // a custom string displaying a status
    MSG_RET_PLAYERS_STREAM,
    MSG_RET_INFO,
    MSG_RET_GAMES_STREAM,
    MSG_RET_GAME_BOARD,
    MSG_RET_DEFY_NOTIFY,
    MSG_RET_OBSERVER_NOTIFY,
    MSG_RET_WINNER,
    MSG_RET_FRIENDS_STREAM,
    MSG_RET_HISTORY_STREAM,
    MSG_RET_END
} MsgServerType;

//  INSA in ascii => hexadecimal
// this is used to check 2 things:
// we are receiving a message expected and the endianness is correct
// this is done in protocol resolution in general and is a good way to check if the message is valid

typedef struct
{
    Username p1;
    Username p2;
    int id;
    Username winner;
} MsgGame;

typedef struct
{
    uint64_t magic;
    MsgServerType kind;

    union
    {
        struct
        {
            bool is_error;
            char msg[1024];
        } status;

        struct
        {
            bool is_leaderboard;
            Username names[16];
            int elos[16];
        } onlinePlayersSet, leaderboards; // STREAMED

        struct
        {
            Username name;
            char bio[256];
            int rank;
        } playerInfo;

        struct
        {
            MsgGame games[8];
        } gamesSet; // STREAMED

        struct
        {
            int board[12];
            int hands[2];
            Username p1;
            Username p2;
            int currentPlayer;
        } gameBoard;

        struct
        {
            Username defier;
        } defyNotify;

        struct
        {
            Username spectator;
        } observerNotify;

        struct
        {
            Username winner;
            bool is_draw;
        } winnerNotify;

        struct
        {
            Username friends[16];
        } friendsSet; // STREAMED

        struct
        {
            MsgGame games[8];
        } gamesSetHistory; // STREAMED
    };
} Server2ClientMessage;
