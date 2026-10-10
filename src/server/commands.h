#include <string.h>
#include "server/client2.h"
#include "server/game.h"
#include "server/message.h"
#include "server/models/games.h"
#include "server/server2.h"
#include "shared/models/clientServer.h"

void handle_message(Client *listeClients, Client *client, int clientsCount, char *buffer, Games *games);
