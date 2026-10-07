#include "server/commands.h"

void handle_message(Client *listeClients, Client *client, int *clientsCount, char *buffer)
{
    // Nettoyage
    buffer[strcspn(buffer, "\r\n")] = 0;
    if (buffer[0] == 0)
        return;

    ClientServerMessage message;
    if (parse_message(buffer, &message) < 0)
    {
        write_client(client->sock, "C'est quoi cte commande ? pas dans les specs frérot");
        return;
    }

    switch (message.kind)
    {
    case MSG_PLAYERS:
        char message[BUF_SIZE] = "Liste des petits filous connectés :\n";
        for (int i = 0; i < *clientsCount; ++i)
        {
            strncat(message, "- ", sizeof message - strlen(message) - 1);
            strncat(message, listeClients[i].name, sizeof message - strlen(message) - 1);
            strncat(message, "\n", sizeof message - strlen(message) - 1);
        }
        write_client(client->sock, message);
        break;
    }
}