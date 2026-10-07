#include "server/commands.h"

void handle_message(Client *listeClients, Client *client, int *clientsCount, char *buffer, Games *games)
{
    // Nettoyage
    buffer[strcspn(buffer, "\r\n")] = 0;
    if (buffer[0] == 0)
        return;

    ClientServerMessage message;
    if (parse_message(buffer, &message) < 0)
    {
        write_client(client->sock, "C'est quoi cte commande ? pas dans les specs frérot\n");
        return;
    }

    switch (message.kind)
    {
    case MSG_PLAYERS:
    {
        char response[BUF_SIZE] = "Liste des petits filous connectés :\n";
        for (int i = 0; i < *clientsCount; ++i)
        {
            strncat(response, "- ", sizeof response - strlen(response) - 1);
            strncat(response, listeClients[i].name, sizeof response - strlen(response) - 1);
            strncat(response, "\n", sizeof response - strlen(response) - 1);
        }
        write_client(client->sock, response);
        break;
    }
    case MSG_MESSAGE:
        send_message_to_all_clients(listeClients, *client, *clientsCount, message.message.message, 0);
        break;
    case MSG_DEFY:
    {
        Client *adversaire = NULL;
        for (int i = 0; i < *clientsCount; ++i)
        {
            if (strcmp(listeClients[i].name, message.defy.pseudo) == 0)
                adversaire = &listeClients[i];
        }
        if (adversaire == NULL || adversaire->sock == client->sock)
        {
            write_client(client->sock, "Cki?\n");
            break;
        }
        // Check if defier is already in game
        for (int i = 0; i < games->count; ++i)
        {
            if (games->games[i].status == WAITING)
                continue;
            if (
                strcmp(games->games[i].playerNames[0], message.defy.pseudo) == 0 || strcmp(games->games[i].playerNames[1], message.defy.pseudo) == 0)
            {
                write_client(client->sock, "T'es déjà en game frérot, essaie déjà de gagner celle là sale fou\n");
                return;
            }
        }

        Game newGame = {0};
        gameInit(&newGame);
        if (vec_expand_((char **)&games->games, &games->count, &games->capacity, sizeof(Game)) != 0)
        {
            write_client(client->sock, "euh j'ai un bug déso c'est finito\n");
            break;
        }
        strncpy(newGame.playerNames[0], client->name, sizeof newGame.playerNames[0] - 1);
        strncpy(newGame.playerNames[1], adversaire->name, sizeof newGame.playerNames[1] - 1);
        games->games[games->count] = newGame;
        ++games->count;

        char invitation[BUF_SIZE] = "1V1 NO RE CONTRE ";
        strncat(invitation, client->name, sizeof invitation - strlen(invitation) - 1);
        strncat(invitation, " ?\n", sizeof invitation - strlen(invitation) - 1);
        write_client(adversaire->sock, invitation);
        break;
    }
    default:
        break;
    }
}