#pragma once

#include "server/player.h"
#include "shared/vec.h"

#define DEFAULT_GAME_HISTORY_PATH "game_history.json"

// player play
typedef vec_t(int) Turns;

typedef struct GameHistory
{
    Turns turns;
    PlayerId p1;
    PlayerId p2;
    PlayerId winner;
} GameHistory;

void gameHistoryInit(const char *path);

void gameHistoryPersist(GameHistory *hist);
void gameHistoryForEach(void (*callback)(GameHistory *elt, void *ctx), void *ctx);
