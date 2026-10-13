#pragma once

#include "server/client2.h"
#include "shared/game.h"
#include "shared/models/clientServer.h"

void handle_message(Clients *listeClients, Client *client, ClientServerMessage *msg, Games *games);

void signal_disconnect(Clients *listeClients, Client *client, Games *games);
