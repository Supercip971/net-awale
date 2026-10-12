#pragma once

#include "server/common.h"
#include "shared/models/clientServer.h"

void write_client(SOCKET sock, const char *buffer);
