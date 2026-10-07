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
        char response[BUF_SIZE] = "Liste des petits filous connectés :\n";
        for (int i = 0; i < *clientsCount; ++i)
        {
            strncat(response, "- ", sizeof response - strlen(response) - 1);
            strncat(response, listeClients[i].name, sizeof response - strlen(response) - 1);
            strncat(response, "\n", sizeof response - strlen(response) - 1);
        }
        write_client(client->sock, response);
        break;
    case MSG_MESSAGE:
        send_message_to_all_clients(listeClients, *client, *clientsCount, message.message.message, 0);
        break;
    }
}