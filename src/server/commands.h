#pragma once

#include "server/client2.h"
#include "server/models/games.h"

void handle_message(Clients *listeClients, Client *client, char *buffer, Games *games);
