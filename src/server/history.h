#pragma once

#include "shared/game.h"
#include "shared/vec.h"

#define DEFAULT_GAME_HISTORY_PATH "game_history.json"

typedef struct Turn
{
    // TODO: fill this
    char *play;
} Turn;

typedef vec_t(Turn) Turns;

typedef struct GameHistory
{
    Turns turns;
    char *p1;
    char *p2;
    int winner;
} GameHistory;

void gameHistoryInit(const char *path);

void gameHistoryPersist(GameHistory *hist);
void gameHistoryForEach(void (*callback)(GameHistory *elt, void *ctx), void *ctx);
