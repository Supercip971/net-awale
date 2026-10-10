#pragma once

#include <stdio.h>
#include "server/common.h"
#include "shared/game.h"

typedef enum
{
    GAME_ONGOING = 0,
    GAME_WIN_P0,
    GAME_WIN_P1,
    GAME_DRAW
} GameStatus;

void gameInit(Game *game);

// Returns 1 if the move was played, 0 if it is illegal (game untouched).
// capturedSeeds receives the total seeds captured by this move.
int play(Game *game, int player, int hole, int *capturedSeeds);

// Returns 1 if `player` has at least one legal move.
int hasLegalMove(const Game *game, int player);

// Call after each move, with the player who must play next.
// If nobody can move, remaining seeds are given to the owner of the side
// they are on (mutates game->hands). Returns the game state.
GameStatus gameStatus(Game *game, int nextPlayer);

char *printGame(Game *game, char board[BUF_SIZE]);

char *displayGame(Game *game, char *response, size_t size);
