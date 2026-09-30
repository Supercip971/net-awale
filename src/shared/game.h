#pragma once

typedef struct Game
{
    // player 1: first 6
    // player 2: last 6
    int board[12];
    int hands[2];
    char *p1;
    char *p2;
} Game;
