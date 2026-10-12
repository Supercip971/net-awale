#pragma once
#include "server/player.h"
#include "shared/models/const.h"
#include "shared/vec.h"

typedef enum
{
    WAITING,
    IN_GAME
} GAME_STATUS;

typedef vec_t(int) Turns;

typedef struct Game
{
    // player 1: first 6
    // player 2: last 6
    int board[12];
    int hands[2];
    int currentPlayer; // 0 for player 1, 1 for player 2
    PlayerId players[2];
    PlayerId spectators[MAX_SPECTATORS];
    int spectatorsCount;
    GAME_STATUS status;
    Turns turns;
    int private;
} Game;

typedef vec_t(Game) Games;
