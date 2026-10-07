#include <string.h>
#include "server/client2.h"
#include "server/message.h"
#include "shared/models/clientServer.h"

void handle_message(Client *listeClients, Client *client, int *clientsCount, char *buffer);