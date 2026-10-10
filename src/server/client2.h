#pragma once

#include "server/common.h"
#include "server/player.h"

typedef struct
{
    SOCKET sock;
    PlayerId player;
} Client;
