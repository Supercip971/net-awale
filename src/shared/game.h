#pragma once

typedef struct Game
{
    // player 1: first 6
    // player 2: last 6
    int board[12];
    int hands[2];
    int currentPlayer;    // 0 for player 1, 1 for player 2
    char *playerNames[2]; // player names
} Game;
