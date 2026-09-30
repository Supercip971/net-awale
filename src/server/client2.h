#pragma once

#include "server/common.h"

typedef struct
{
    SOCKET sock;
    char name[BUF_SIZE];
} Client;
