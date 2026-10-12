#pragma once

#include "server/common.h"
#include "server/history.h"
#include "server/player.h"

typedef struct
{
    SOCKET sock;
    PlayerId player;
    int currentReplayStep;
    GameHistoryId currentGameHistoryId;
} Client;

typedef vec_t(Client) Clients;
