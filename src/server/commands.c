#include "server/commands.h"

int checkOpponent(Client **adversaire, int clientsCount, char pseudo[MAX_USERNAME_LENGTH], Client *listeClients, Client *client)
{
    for (int i = 0; i < clientsCount; ++i)
    {
        if (strcmp(listeClients[i].name, pseudo) == 0)
        {
            *adversaire = &listeClients[i];
            break;
        }
    }
    if (*adversaire == NULL || (*adversaire)->sock == client->sock)
    {
        write_client(client->sock, "Cki?\n");
        return 0;
    }
    return 1;
}

int findOpponentNameAndGame(Client **adversaire, int clientsCount, Client *listeClients, Games *games, Client *client, Game **game)
{
    char opponentName[MAX_USERNAME_LENGTH] = {0};
    for (int i = 0; i < games->count; ++i)
    {
        if (games->games[i].status != IN_GAME)
            continue;
        if (strcmp(games->games[i].playerNames[0], client->name) == 0)
        {
            strncpy(opponentName, games->games[i].playerNames[1], sizeof opponentName - 1);
            *game = &(games->games[i]);
        }
        else if (strcmp(games->games[i].playerNames[1], client->name) == 0)
        {
            strncpy(opponentName, games->games[i].playerNames[0], sizeof opponentName - 1);
            *game = &(games->games[i]);
        }
    }
    if (opponentName[0] == 0)
    {
        return 0;
    }
    for (int i = 0; i < clientsCount; ++i)
    {
        if (strcmp(listeClients[i].name, opponentName) == 0)
        {
            *adversaire = &listeClients[i];
            break;
        }
    }
    if (*adversaire == NULL || (*adversaire)->sock == client->sock)
    {
        return 0;
    }
    return 1;
}

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
        if (!checkOpponent(&adversaire, *clientsCount, message.defy.pseudo, listeClients, client))
            return;

        // Check if defier is already in game
        for (int i = 0; i < games->count; ++i)
        {
            if (games->games[i].status == IN_GAME &&
                (strcmp(games->games[i].playerNames[0], (adversaire)->name) == 0 ||
                 strcmp(games->games[i].playerNames[1], (adversaire)->name) == 0))
            {
                write_client(client->sock, "Il est déjà en game, attends ton tour\n");
                return;
            }
            if (games->games[i].status == WAITING &&
                strcmp(games->games[i].playerNames[0], (adversaire)->name) == 0 &&
                strcmp(games->games[i].playerNames[1], client->name) == 0)
            {
                write_client(client->sock, "Il t'a déjà défié, accepte ou refuse\n");
                return;
            }
            // L'initiateur de la demande est toujours à l'index 0
            if (
                (games->games[i].status == WAITING && strcmp(games->games[i].playerNames[0], client->name) == 0) ||
                (games->games[i].status == IN_GAME &&
                 (strcmp(games->games[i].playerNames[0], client->name) == 0 ||
                  strcmp(games->games[i].playerNames[1], client->name) == 0)))
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
    case MSG_DECLINE_DEFY:
    {
        Client *adversaire = NULL;
        if (!checkOpponent(&adversaire, *clientsCount, message.declineDefy.pseudo, listeClients, client))
            return;

        // Vu qu'on a qu'une seule game par joueur, on peut supprimer que le premier qu'on trouve
        int found = 0;
        for (int i = 0; i < games->count; ++i)
        {
            if (games->games[i].status != WAITING)
                continue;
            if ((strcmp(games->games[i].playerNames[0], adversaire->name) == 0 &&
                 strcmp(games->games[i].playerNames[1], client->name) == 0))
            {
                vec_splice_((char **)&games->games, &games->count, &games->capacity, sizeof(Game), i, 1);
                --games->count;
                found = 1;
                break;
            }
        }
        if (!found)
        {
            write_client(client->sock, "Tu déclines quoi là frr ? personne t'a défié retourne dodo.\n");
            return;
        }

        char invitation[BUF_SIZE] = "déso, ";
        strncat(invitation, client->name, sizeof invitation - strlen(invitation) - 1);
        strncat(invitation, " a trop peur, il a refusé\n", sizeof invitation - strlen(invitation) - 1);
        write_client(adversaire->sock, invitation);
        break;
    }
    case MSG_ACCEPT_DEFY:
    {
        Client *adversaire = NULL;
        if (!checkOpponent(&adversaire, *clientsCount, message.declineDefy.pseudo, listeClients, client))
            return;

        // Check if defier is already in game
        for (int i = 0; i < games->count; ++i)
        {
            if (games->games[i].status == IN_GAME &&
                (strcmp(games->games[i].playerNames[0], (adversaire)->name) == 0 ||
                 strcmp(games->games[i].playerNames[1], (adversaire)->name) == 0))
            {
                write_client(client->sock, "Il est déjà en game, attends ton tour\n");
                return;
            }
            // L'initiateur de la demande est toujours à l'index 0
            if (
                (games->games[i].status == WAITING && strcmp(games->games[i].playerNames[0], client->name) == 0) ||
                (games->games[i].status == IN_GAME &&
                 (strcmp(games->games[i].playerNames[0], client->name) == 0 ||
                  strcmp(games->games[i].playerNames[1], client->name) == 0)))
            {
                write_client(client->sock, "T'es déjà en game frérot, essaie déjà de gagner celle là sale fou\n");
                return;
            }
        }

        // Vu qu'on a qu'une seule game par joueur, on peut récupérer que le premier qu'on trouve
        Game *game = NULL;
        for (int i = 0; i < games->count; ++i)
        {
            if (games->games[i].status != WAITING)
                continue;
            if ((strcmp(games->games[i].playerNames[0], adversaire->name) == 0 &&
                 strcmp(games->games[i].playerNames[1], client->name) == 0))
            {
                game = &(games->games[i]);
                break;
            }
        }
        if (game == NULL)
        {
            write_client(client->sock, "Tu acceptes quoi là frr ? personne t'a défié retourne dodo.\n");
            return;
        }
        game->status = IN_GAME;
        char board[BUF_SIZE] = {0};
        write_client(adversaire->sock, printGame(game, board));
        write_client(client->sock, printGame(game, board));
        break;
    }
    case MSG_PLAY:
    {
        Client *adversaire = NULL;
        Game *game = NULL;
        if (!findOpponentNameAndGame(&adversaire, *clientsCount, listeClients, games, client, &game))
        {
            write_client(client->sock, "T'es tout seul frérot, va défier quelqu'un.\n");
            return;
        }
        if (game == NULL)
        {
            write_client(client->sock, "Tu fais quoi ???? lance une game avant de jouer peut-être ?.\n");
            return;
        }
        int capturedSeeds = 0;
        int currentPlayerIndex = 0;
        if (strcmp(game->playerNames[1], client->name) == 0)
        {
            currentPlayerIndex = 1;
        }
        if (play(game, currentPlayerIndex, message.play.hole, &capturedSeeds))
        {
            game->currentPlayer = game->currentPlayer ? 0 : 1;
            char board[BUF_SIZE] = {0};
            write_client(adversaire->sock, printGame(game, board));
            write_client(client->sock, printGame(game, board));
        }
        else
        {
            write_client(client->sock, "Joue bien, frr respecte les règle un moment.\n");
        }
        break;
    }
    default:
        break;
    }
}