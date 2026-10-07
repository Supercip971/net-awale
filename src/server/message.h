#include <stdio.h>
#include <string.h>
#include "server/common.h"
#include "shared/models/clientServer.h"

void write_client(SOCKET sock, const char *buffer);
int parse_message(const char *buffer, ClientServerMessage *msg);