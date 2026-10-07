#include "server/commands.h"

void handle_message(Client *listeClients, Client *client, int *clientsCount, char *buffer)
{
    // Nettoyage
    buffer[strcspn(buffer, "\r\n")] = 0;
    if (buffer[0] == 0)
        return;

    if (strcmp(buffer, "connect") == 0)
    {
        // TODO
    }
    else if (strcmp(buffer, "players") == 0)
    {
        char message[BUF_SIZE] = "Liste des petits filous connectés :\n";
        for (int i = 0; i < *clientsCount; ++i)
        {
            strncat(message, "- ", sizeof message - strlen(message) - 1);
            strncat(message, listeClients[i].name, sizeof message - strlen(message) - 1);
            strncat(message, "\n", sizeof message - strlen(message) - 1);
        }
        write_client(client->sock, message);
    }
}