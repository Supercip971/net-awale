#pragma once
#include "shared/models/const.h"
#include "shared/game.h"

typedef struct Games
{
    Game* games;
    int count;
    int capacity;
} Games;
