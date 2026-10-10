#include "server/commands.h"

// TODO: Refactor with a map player: socket (avoiding n loops)

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

int find_game(Games *games, char player[MAX_USERNAME_LENGTH], Game **game)
{
    for (int i = 0; i < games->count; ++i)
    {
        if (games->games[i].status != IN_GAME)
            continue;
        if ((strcmp(games->games[i].playerNames[0], player) == 0 ||
             strcmp(games->games[i].playerNames[1], player) == 0))
        {
            *game = &(games->games[i]);
            return 1;
        }
    }
    return 0;
}

int is_player_spectating_this_game(Game *game, char playerName[MAX_USERNAME_LENGTH])
{
    for (int i = 0; i < game->spectatorsCount; ++i)
    {
        if (strcmp(game->spectatorNames[i], playerName) == 0)
        {
            return 1;
        }
    }
    return 0;
}

int is_player_spectating_a_game(Games *games, char playerName[MAX_USERNAME_LENGTH], Game **game)
{
    for (int i = 0; i < games->count; ++i)
    {
        if (is_player_spectating_this_game(&(games->games[i]), playerName))
        {
            *game = &(games->games[i]);
            return 1;
        }
    }
    return 0;
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

int is_player_in_game(Games *games, char name[MAX_USERNAME_LENGTH])
{
    for (int i = 0; i < games->count; ++i)
    {
        if (games->games[i].status != IN_GAME)
            continue;
        if (
            strcmp(games->games[i].playerNames[0], name) == 0 ||
            strcmp(games->games[i].playerNames[1], name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

void msg_players(Client *listeClients, Client *client, int *clientsCount)
{
    char response[BUF_SIZE] = "Liste des petits filous connectés :\n";
    for (int i = 0; i < *clientsCount; ++i)
    {
        strncat(response, "- ", sizeof response - strlen(response) - 1);
        strncat(response, listeClients[i].name, sizeof response - strlen(response) - 1);
        strncat(response, "\n", sizeof response - strlen(response) - 1);
    }
    write_client(client->sock, response);
}

void msg_games(Games *games, Client *client)
{
    unsigned int runningGamesCount = 0;
    char response[BUF_SIZE] = "Liste des COMBATS DE TITANS :\n";
    for (int i = 0; i < games->count; ++i)
    {
        if (games->games[i].status != IN_GAME)
            continue;
        char gameDisplay[BUF_SIZE] = {0};
        displayGame(&(games->games[i]), gameDisplay, sizeof gameDisplay);
        strncat(response, "- ", sizeof response - strlen(response) - 1);
        strncat(response, gameDisplay, sizeof response - strlen(response) - 1);
        strncat(response, "\n", sizeof response - strlen(response) - 1);
        ++runningGamesCount;
    }
    if (!runningGamesCount)
    {
        char noGameResponse[BUF_SIZE] = "Aucun combat de titan. Lances-en un dcp stp\n";
        write_client(client->sock, noGameResponse);
        return;
    }
    write_client(client->sock, response);
}

void msg_message(Client *listeClients, Client *client, int *clientsCount, ClientServerMessage *message)
{
    send_message_to_all_clients(listeClients, *client, *clientsCount, message->message.message, 0);
}

void msg_accept_defy(Client *listeClients, Client *client, int *clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    if (!checkOpponent(&adversaire, *clientsCount, message->acceptDefy.pseudo, listeClients, client))
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

    Game *spectatedGame = NULL;
    if (is_player_spectating_a_game(games, client->name, &spectatedGame))
    {
        write_client(client->sock, "T'es déjà en train de mater une autre game frérot chillax\n");
        return;
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
}

void msg_defy(Client *listeClients, Client *client, int *clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    if (!checkOpponent(&adversaire, *clientsCount, message->defy.pseudo, listeClients, client))
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
            message->kind = MSG_ACCEPT_DEFY;
            strncpy(message->acceptDefy.pseudo, adversaire->name, sizeof message->acceptDefy.pseudo - 1);
            msg_accept_defy(listeClients, client, clientsCount, games, message);
            return;
        }
        // L'initiateur de la demande est toujours à l'index 0
        if (
            (games->games[i].status == IN_GAME &&
             (strcmp(games->games[i].playerNames[0], client->name) == 0 ||
              strcmp(games->games[i].playerNames[1], client->name) == 0)))
        {
            write_client(client->sock, "T'es déjà en game frérot, essaie déjà de gagner celle là sale fou\n");
            return;
        }
    }

    Game *spectatedGame = NULL;
    if (is_player_spectating_a_game(games, client->name, &spectatedGame))
    {
        write_client(client->sock, "T'es déjà en train de mater une autre game frérot chillax\n");
        return;
    }

    Game newGame = {0};
    gameInit(&newGame);
    if (vec_expand_((char **)&games->games, &games->count, &games->capacity, sizeof(Game)) != 0)
    {
        write_client(client->sock, "euh j'ai un bug déso c'est finito\n");
        return;
    }
    strncpy(newGame.playerNames[0], client->name, sizeof newGame.playerNames[0] - 1);
    strncpy(newGame.playerNames[1], adversaire->name, sizeof newGame.playerNames[1] - 1);
    games->games[games->count] = newGame;
    ++games->count;

    char invitation[BUF_SIZE] = "1V1 NO RE CONTRE ";
    strncat(invitation, client->name, sizeof invitation - strlen(invitation) - 1);
    strncat(invitation, " ?\n", sizeof invitation - strlen(invitation) - 1);
    write_client(adversaire->sock, invitation);
}

void msg_decline_defy(Client *listeClients, Client *client, int *clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    if (!checkOpponent(&adversaire, *clientsCount, message->declineDefy.pseudo, listeClients, client))
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
}

int get_players_clients(Client *listeClients, int clientsCount, Game *game, Client **opponent1, Client **opponent2)
{
    for (int i = 0; i < clientsCount; ++i)
    {
        if (strcmp(listeClients[i].name, game->playerNames[0]) == 0)
        {
            *opponent1 = &listeClients[i];
        }
        else if (strcmp(listeClients[i].name, game->playerNames[1]) == 0)
        {
            *opponent2 = &listeClients[i];
        }
    }
    if (*opponent1 == NULL || *opponent2 == NULL)
    {
        return 0;
    }
    return 1;
}

void msg_spec(Client *listeClients, Client *client, int *clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire1 = NULL;
    Client *adversaire2 = NULL;
    if (!checkOpponent(&adversaire1, *clientsCount, message->declineDefy.pseudo, listeClients, client))
        return;

    if (is_player_in_game(games, client->name))
    {
        write_client(client->sock, "T'es déjà en game frérot, chillax\n");
        return;
    }

    Game *game = NULL;
    if (!find_game(games, adversaire1->name, &game))
    {
        write_client(client->sock, "Il est pas en game mdr\n");
        return;
    }

    if (game->spectatorsCount >= MAX_SPECTATORS)
    {
        write_client(client->sock, "Trop de spectateurs, va mater un autre combat frérot\n");
        return;
    }

    if (is_player_spectating_this_game(game, client->name))
    {
        write_client(client->sock, "T'es déjà en train de mater frérot\n");
        return;
    }

    Game *spectatedGame = NULL;
    if (is_player_spectating_a_game(games, client->name, &spectatedGame))
    {
        write_client(client->sock, "T'es déjà en train de mater une autre game frérot chillax\n");
        return;
    }

    if (!get_players_clients(listeClients, *clientsCount, game, &adversaire1, &adversaire2))
    {
        write_client(client->sock, "Les joueurs existent pas frr (ils ont dû se déco entre temps, réessaie)\n");
        return;
    }

    strncpy(game->spectatorNames[game->spectatorsCount], client->name, sizeof game->spectatorNames[game->spectatorsCount - 1] - 1);
    ++game->spectatorsCount;

    char messageSpec[BUF_SIZE] = "";
    strncat(messageSpec, client->name, sizeof messageSpec - strlen(messageSpec) - 1);
    strncat(messageSpec, " se cache dans les buissons pour vous observer...\n", sizeof messageSpec - strlen(messageSpec) - 1);
    write_client(adversaire1->sock, messageSpec);
    write_client(adversaire2->sock, messageSpec);
    write_client(client->sock, "T'es maintenant en mode spectateur, enjoy le spectacle frérot\n");
}

void msg_stop_spec(Client *client, Games *games)
{
    Game *game = NULL;
    if (!is_player_spectating_a_game(games, client->name, &game))
    {
        write_client(client->sock, "T'es pas en train de mater une game frérot\n");
        return;
    }
    for (int i = 0; i < game->spectatorsCount; ++i)
    {
        if (strcmp(game->spectatorNames[i], client->name) == 0)
        {
            vec_splice_((char **)game->spectatorNames, &game->spectatorsCount, NULL, MAX_USERNAME_LENGTH, i, 1);
            --game->spectatorsCount;
            break;
        }
    }
    write_client(client->sock, "T'as quitté la partie c'est good\n");
}

void msg_play(Client *listeClients, Client *client, int *clientsCount, Games *games, ClientServerMessage *message)
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
    if (currentPlayerIndex != game->currentPlayer)
    {
        write_client(client->sock, "Chill bro, laisse le jouer le pauvre.\n");
        return;
    }
    if (play(game, currentPlayerIndex, message->play.hole, &capturedSeeds))
    {
        game->currentPlayer = game->currentPlayer ? 0 : 1;
        char board[BUF_SIZE] = {0};
        write_client(adversaire->sock, printGame(game, board));
        write_client(client->sock, printGame(game, board));
        for (int i = 0; i < game->spectatorsCount; ++i)
        {
            for (int j = 0; j < *clientsCount; ++j)
            {
                if (strcmp(listeClients[j].name, game->spectatorNames[i]) == 0)
                {
                    write_client(listeClients[j].sock, printGame(game, board));
                    break;
                }
            }
        }
        GameStatus result = gameStatus(game, game->currentPlayer);
        if (result == GAME_WIN_P0)
        {
            write_client(adversaire->sock, "\nT'as gagné frérot, gg.\n");
            write_client(client->sock, "\nT'as perdu frérot, laonte.\n");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < *clientsCount; ++j)
                {
                    if (strcmp(listeClients[j].name, game->spectatorNames[i]) == 0)
                    {
                        char spectatorMessage[BUF_SIZE] = {0};
                        strncat(spectatorMessage, adversaire->name, sizeof spectatorMessage - strlen(spectatorMessage) - 1);
                        strncat(spectatorMessage, " a gagné, gg ez pour lui.\n", sizeof spectatorMessage - strlen(spectatorMessage) - 1);
                        write_client(listeClients[j].sock, spectatorMessage);
                        break;
                    }
                }
            }
        }
        else if (result == GAME_WIN_P1)
        {
            write_client(adversaire->sock, "\nT'as perdu frérot, laonte.\n");
            write_client(client->sock, "\nT'as gagné frérot, gg.\n");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < *clientsCount; ++j)
                {
                    if (strcmp(listeClients[j].name, game->spectatorNames[i]) == 0)
                    {
                        char spectatorMessage[BUF_SIZE] = {0};
                        strncat(spectatorMessage, client->name, sizeof spectatorMessage - strlen(spectatorMessage) - 1);
                        strncat(spectatorMessage, " a gagné, gg ez pour lui.\n", sizeof spectatorMessage - strlen(spectatorMessage) - 1);
                        write_client(listeClients[j].sock, spectatorMessage);
                        break;
                    }
                }
            }
        }
        else if (result == GAME_DRAW)
        {
            write_client(adversaire->sock, "\nEgalité frérot, nul.\n");
            write_client(client->sock, "\nEgalité frérot, nul.\n");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < *clientsCount; ++j)
                {
                    if (strcmp(listeClients[j].name, game->spectatorNames[i]) == 0)
                    {
                        write_client(listeClients[j].sock, "\nT'as spectate pour r, ils ont fait égalité ces nuls\n");
                        break;
                    }
                }
            }
        }
        if (result != GAME_ONGOING)
        {
            for (int i = 0; i < games->count; ++i)
            {
                if (&(games->games[i]) == game)
                {
                    vec_splice_((char **)&games->games, &games->count, &games->capacity, sizeof(Game), i, 1);
                    --games->count;
                    break;
                }
            }
        }
    }
    else
    {
        write_client(client->sock, "Joue bien, frr respecte les règle un moment.\n");
    }
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
        msg_players(listeClients, client, clientsCount);
        break;
    case MSG_GAMES:
        msg_games(games, client);
        break;
    case MSG_MESSAGE:
        msg_message(listeClients, client, clientsCount, &message);
        break;
    case MSG_DEFY:
        msg_defy(listeClients, client, clientsCount, games, &message);
        break;
    case MSG_DECLINE_DEFY:
        msg_decline_defy(listeClients, client, clientsCount, games, &message);
        break;
    case MSG_ACCEPT_DEFY:
        msg_accept_defy(listeClients, client, clientsCount, games, &message);
        break;
    case MSG_PLAY:
        msg_play(listeClients, client, clientsCount, games, &message);
        break;
    case MSG_SPEC:
        msg_spec(listeClients, client, clientsCount, games, &message);
        break;
    case MSG_STOP_SPEC:
        msg_stop_spec(client, games);
        break;
    case MSG_SETBIO:
    {
        // TODO: Save bio in persistence
        break;
    }
    default:
        break;
    }
}