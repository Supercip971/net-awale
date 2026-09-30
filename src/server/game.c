#include "game.h"

void gameInit(Game *game)
{
    // init at 0
    *game = (Game){};

    // 4 seed per hole
    for (int i = 0; i < 12; i++)
        game->board[i] = 4;

    // 0 seed per hand
}
