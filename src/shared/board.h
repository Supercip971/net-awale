#pragma once

typedef struct Board
{
    // player 1: first 6
    // player 2: last 6

    int holes[12];
    int hands[2];
} Board;

void boardInit(Board *);

void boardPrint(Board *);
