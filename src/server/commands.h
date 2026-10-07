#include <string.h>
#include "server/client2.h"
#include "server/message.h"

void handle_message(Client *listeClients, Client *client, int *clientsCount, char *buffer);