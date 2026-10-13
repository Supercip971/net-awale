#include "server/elo.h"
#include <math.h>
#include "server/history.h"

void updateElos(Player *p1, Player *p2, GAME_RESULT result)
{
    double differenceRank = (double)(p1->rank - p2->rank);
    double probaWinP1 = 1.0 / (1.0 + pow(10.0, -differenceRank / 400.0));
    double probaWinP2 = 1.0 - probaWinP1;
    int k1 = p1->rank > 2400 ? 10 : 20;
    int k2 = p2->rank > 2400 ? 10 : 20;
    if (countGameHistoryForPlayer(p1->user_id) <= 30)
        k1 = 40;
    if (countGameHistoryForPlayer(p2->user_id) <= 30)
        k2 = 40;
    double scoreP1 = result == GAME_RESULT_DRAW ? 0.5 : (result == P1_WIN ? 1.0 : 0.0);
    double scoreP2 = 1.0 - scoreP1;
    p1->rank += (int)round(k1 * (scoreP1 - probaWinP1));
    p2->rank += (int)round(k2 * (scoreP2 - (probaWinP2)));
}