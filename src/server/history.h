#pragma once

#include "server/player.h"
#include "shared/game.h"
#include "shared/vec.h"

#define DEFAULT_GAME_HISTORY_PATH "game_history.json"

// player play

typedef uint64_t GameHistoryId;
typedef struct GameHistory
{
    GameHistoryId id;
    Turns turns;
    PlayerId p1;
    PlayerId p2;
    PlayerId winner;
} GameHistory;

typedef vec_t(GameHistory) GameHistories;

void gameHistoryInit();
void gameHistoryDeinit();

void gameHistoryGameEnded(Game *game, int winner);

void gameHistoryForEach(void (*callback)(GameHistory *elt, void *ctx), void *ctx, int count);
void gameHistoryPlayerForEach(PlayerId of, void (*callback)(GameHistory *elt, void *ctx), void *ctx, int count);
