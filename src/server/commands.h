#pragma once

#include "server/client2.h"
#include "server/models/games.h"

void handle_message(Client *listeClients, Client *client, int clientsCount, char *buffer, Games *games);
