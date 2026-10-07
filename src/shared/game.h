#pragma once
#include "shared/models/const.h"

typedef enum
{
    WAITING,
    IN_GAME
} GAME_STATUS;

typedef struct Game
{
    // player 1: first 6
    // player 2: last 6
    int board[12];
    int hands[2];
    int currentPlayer;                        // 0 for player 1, 1 for player 2
    char playerNames[2][MAX_USERNAME_LENGTH]; // player names
    GAME_STATUS status;
} Game;
