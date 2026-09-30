#include "shared/board.h"

void boardInit(Board * board)
{
    // init at 0
    *board = (Board){};

    // 4 seed per hole
    for (int i = 0; i < 12; i++)
        board->holes[i] = 4;

    // 0 seed per hand
}
