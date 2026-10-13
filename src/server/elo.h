#pragma once
#include "server/player.h"

#define INITIAL_ELO 1000

typedef enum
{
    P1_WIN = 0,
    P2_WIN = 1,
    GAME_RESULT_DRAW = 2,
} GAME_RESULT;

void updateElos(Player *p1, Player *p2, GAME_RESULT result);
