#include "server/commands.h"
#include <stdlib.h>
#include <string.h>
#include "server/models/games.h"
#include "server/player.h"
#include "shared/game.h"
#include "shared/vec.h"

// TODO: Refactor with a map player: socket (avoiding n loops)

int checkOpponent(Client **adversaire, int clientsCount, char pseudo[MAX_USERNAME_LENGTH], Client *listeClients, Client *client)
{
    Player *player = find_player_by_name(pseudo);

    if (player == NULL)
    {
        write_client(client->sock, "Cki?\n");
        return 0;
    }

    if (player->user_id == client->player)
    {
        write_client(client->sock, "Cki?\n");
        return 0;
    }

    for (int i = 0; i < clientsCount; ++i)
    {
        if (listeClients[i].player == player->user_id)
        {
            *adversaire = &listeClients[i];
            return 1;
        }
    }

    write_client(client->sock, "Il n'est pas en ligne\n");
    return 0;
}

Game *find_game(Games *games, PlayerId player)
{
    for (int i = 0; i < games->length; ++i)
    {
        Game *cur = &games->data[i];
        if (cur->status != IN_GAME)
            continue;

        if (cur->players[0] == player || cur->players[1] == player)
        {
            return cur;
        }
    }
    return NULL;
}

int is_player_spectating_this_game(Game *game, PlayerId player)
{
    for (int i = 0; i < game->spectatorsCount; ++i)
    {
        if (game->spectators[i] == player)
        {
            return 1;
        }
    }
    return 0;
}

Game *is_player_spectating_a_game(Games *games, PlayerId player)
{
    for (int i = 0; i < games->length; ++i)
    {
        if (is_player_spectating_this_game(&(games->data[i]), player))
        {
            return &games->data[i];
        }
    }
    return NULL;
}

int findOpponentNameAndGame(Client **adversaire, int clientsCount, Client *listeClients, Games *games, Client *client, Game **res)
{
    PlayerId opponent = INVALID_PLAYER_ID;
    for (int i = 0; i < games->length; ++i)
    {
        Game *game = &games->data[i];
        if (game->status != IN_GAME)
            continue;

        if (game->players[0] == client->player)
        {
            opponent = game->players[1];
            *res = game;
        }
        else if (game->players[1] == client->player)
        {
            opponent = game->players[0];
            *res = game;
        }
    }
    if (opponent == INVALID_PLAYER_ID)
    {
        return 0;
    }

    for (int i = 0; i < clientsCount; ++i)
    {
        if (listeClients[i].player == opponent)
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

int is_player_in_game(Games *games, PlayerId player)
{
    for (int i = 0; i < games->length; ++i)
    {
        if (games->data[i].status != IN_GAME)
            continue;

        if (games->data[i].players[0] == player || games->data[i].players[1] == player)
        {
            return 1;
        }
    }
    return 0;
}

void msg_players(Client *listeClients, Client *client, int clientsCount)
{
    char response[BUF_SIZE] = "Liste des petits filous connectés :\n";
    size_t offset = strlen(response);
    for (int i = 0; i < clientsCount; ++i)
    {
        Player *player = find_player_by_id(listeClients[i].player);
        const char *name = player ? player->name : "Anonyme(erroeur)";
        int l = snprintf(response + offset, sizeof response - offset, "- %s\n", name);
        if (l < 0 || (size_t)l >= sizeof response - offset)
            break;
        offset += l;
    }

    write_client(client->sock, response);
}

void msg_player_info(Client* client, const char* pname)
{
    char response[BUF_SIZE] = {};
    Player *player = find_player_by_name(pname);
    if (player)
    {
        int l = snprintf(response, sizeof response, "Information sur le joueur %s :\n"
                "  Bio: %s\n"
                "  Rank: %d\n", player->name, player->bio, player->rank);
        if (l < 0 || (size_t)l >= sizeof response)
            return;
        write_client(client->sock, response);
        return;
    }
    char noPlayerResponse[BUF_SIZE] = "Joueur non trouvé.\n";
    write_client(client->sock, noPlayerResponse);
}

void msg_games(Games *games, Client *client)
{
    unsigned int runningGamesCount = 0;
    char response[BUF_SIZE] = "Liste des COMBATS DE TITANS :\n";
    size_t offset = strlen(response);
    for (int i = 0; i < games->length; ++i)
    {
        if (games->data[i].status != IN_GAME)
            continue;
        char gameDisplay[BUF_SIZE] = {0};
        displayGame(&(games->data[i]), gameDisplay, sizeof gameDisplay);
        int l = snprintf(response + offset, sizeof response - offset, "- %s\n", gameDisplay);
        if (l < 0 || (size_t)l >= sizeof response - offset)
            break;
        offset += l;
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

void msg_message(Client *listeClients, Client *client, int clientsCount, ClientServerMessage *message)
{
    send_message_to_all_clients(listeClients, *client, clientsCount, message->message.message, 0);
}

void msg_accept_defy(Client *listeClients, Client *client, int clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    if (!checkOpponent(&adversaire, clientsCount, message->acceptDefy.pseudo, listeClients, client))
        return;

    // Check if defier is already in game
    for (int i = 0; i < games->length; ++i)
    {
        // We do a pointer car copying a game is heavy
        Game *game = &games->data[i];
        if (game->status == IN_GAME &&
            (game->players[0] == adversaire->player || game->players[1] == adversaire->player))
        {
            write_client(client->sock, "Il est déjà en game, attends ton tour\n");
            return;
        }
        // L'initiateur de la demande est toujours à l'index 0
        if (
            (game->status == WAITING && game->players[0] == client->player) ||
            (game->status == IN_GAME &&
             (game->players[0] == client->player || game->players[1] == client->player)))
        {
            write_client(client->sock, "T'es déjà en game frérot, essaie déjà de gagner celle là sale fou\n");
            return;
        }
    }

    Game *spectatedGame = is_player_spectating_a_game(games, client->player);

    if (spectatedGame != NULL)
    {
        write_client(client->sock, "T'es déjà en train de mater une autre game frérot chillax\n");
        return;
    }

    // Vu qu'on a qu'une seule game par joueur, on peut récupérer que le premier qu'on trouve
    Game *game = NULL;
    for (int i = 0; i < games->length; ++i)
    {
        Game *cur = &games->data[i];
        if (cur->status != WAITING)
            continue;
        if (cur->players[0] == adversaire->player && cur->players[1] == client->player)
        {
            game = cur;
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

void msg_defy(Client *listeClients, Client *client, int clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    if (!checkOpponent(&adversaire, clientsCount, message->defy.pseudo, listeClients, client))
        return;

    // Check if defier is already in game
    for (int i = 0; i < games->length; ++i)
    {
        Game *game = &games->data[i];
        if (game->status == IN_GAME &&
            (game->players[0] == adversaire->player || game->players[1] == adversaire->player))
        {
            write_client(client->sock, "Il est déjà en game, attends ton tour\n");
            return;
        }

        if (game->status == WAITING &&
            game->players[0] == adversaire->player &&
            game->players[1] == client->player)
        {
            message->kind = MSG_ACCEPT_DEFY;

            Player *adversaire_p = find_player_by_id(adversaire->player);
            strncpy(message->acceptDefy.pseudo, adversaire_p ? adversaire_p->name : "", sizeof message->acceptDefy.pseudo - 1);
            message->acceptDefy.pseudo[sizeof message->acceptDefy.pseudo - 1] = 0;
            msg_accept_defy(listeClients, client, clientsCount, games, message);
            return;
        }

        // L'initiateur de la demande est toujours à l'index 0
        if (
            (game->status == WAITING && game->players[0] == client->player) ||
            (game->status == IN_GAME &&
             (game->players[0] == client->player || game->players[1] == client->player)))
        {
            write_client(client->sock, "T'es déjà en game frérot, essaie déjà de gagner celle là sale fou\n");
            return;
        }
    }

    Game *spectatedGame = is_player_spectating_a_game(games, client->player);

    if (spectatedGame)
    {
        write_client(client->sock, "T'es déjà en train de mater une autre game frérot chillax\n");
        return;
    }

    Game newGame = {0};
    gameInit(&newGame);
    newGame.players[0] = client->player;
    newGame.players[1] = adversaire->player;
    vec_push(games, newGame);

    Player *client_p = find_player_by_id(client->player);
    char invitation[BUF_SIZE] = {};
    snprintf(invitation, sizeof invitation, "1V1 NO RE CONTRE %s ?\n", client_p ? client_p->name : "Unknown");

    write_client(adversaire->sock, invitation);
}

void msg_decline_defy(Client *listeClients, Client *client, int clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    if (!checkOpponent(&adversaire, clientsCount, message->declineDefy.pseudo, listeClients, client))
        return;

    // Vu qu'on a qu'une seule game par joueur, on peut supprimer que le premier qu'on trouve
    int found = 0;
    for (int i = 0; i < games->length; ++i)
    {
        Game *game = &games->data[i];

        if (game->status != WAITING)
            continue;
        if (game->players[0] == adversaire->player &&
            game->players[1] == client->player)
        {
            vec_splice(games, i, 1);
            found = 1;
            break;
        }
    }
    if (!found)
    {
        write_client(client->sock, "Tu déclines quoi là frr ? personne t'a défié retourne dodo.\n");
        return;
    }

    Player *client_p = find_player_by_id(client->player);
    char invitations[BUF_SIZE] = {};
    snprintf(invitations, sizeof invitations, "déso, %s a trop peur, il a refusé\n", client_p ? client_p->name : "Unknown");
    write_client(adversaire->sock, invitations);
}

int get_players_clients(Client *listeClients, int clientsCount, Game *game, Client **opponent1, Client **opponent2)
{
    for (int i = 0; i < clientsCount; ++i)
    {
        if (listeClients[i].player == game->players[0])
        {
            *opponent1 = &listeClients[i];
        }
        else if (listeClients[i].player == game->players[1])
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

void msg_spec(Client *listeClients, Client *client, int clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire1 = NULL;
    Client *adversaire2 = NULL;
    if (!checkOpponent(&adversaire1, clientsCount, message->spec.pseudo, listeClients, client))
        return;

    if (is_player_in_game(games, client->player))
    {
        write_client(client->sock, "T'es déjà en game frérot, chillax\n");
        return;
    }

    Game *game = find_game(games, adversaire1->player);
    if (game == NULL)
    {
        write_client(client->sock, "Il est pas en game mdr\n");
        return;
    }

    if (game->spectatorsCount >= MAX_SPECTATORS)
    {
        write_client(client->sock, "Trop de spectateurs, va mater un autre combat frérot\n");
        return;
    }

    if (is_player_spectating_this_game(game, client->player))
    {
        write_client(client->sock, "T'es déjà en train de mater frérot\n");
        return;
    }

    Game *spectatedGame = is_player_spectating_a_game(games, client->player);

    if (spectatedGame != NULL)
    {
        write_client(client->sock, "T'es déjà en train de mater une autre game frérot chillax\n");
        return;
    }

    if (!get_players_clients(listeClients, clientsCount, game, &adversaire1, &adversaire2))
    {
        write_client(client->sock, "Les joueurs existent pas frr (ils ont dû se déco entre temps, réessaie)\n");
        return;
    }

    game->spectators[game->spectatorsCount] = client->player;
    ++game->spectatorsCount;

    Player *client_p = find_player_by_id(client->player);
    const char *name = client_p ? client_p->name : "Un inconnu";
    char messageSpec[BUF_SIZE] = {0};
    snprintf(messageSpec, sizeof messageSpec, "%s se cache dans les buissons pour vous observer...\n", name);
    write_client(adversaire1->sock, messageSpec);
    write_client(adversaire2->sock, messageSpec);
    write_client(client->sock, "T'es maintenant en mode spectateur, enjoy le spectacle frérot\n");
}

void msg_stop_spec(Client *client, Games *games)
{
    Game *game = is_player_spectating_a_game(games, client->player);

    if (game == NULL)
    {
        write_client(client->sock, "T'es pas en train de mater une game frérot\n");
        return;
    }
    for (int i = 0; i < game->spectatorsCount; ++i)
    {
        if (game->spectators[i] == client->player)
        {
            // vec_splice ici fait la même chose en vrai
            memmove(&game->spectators[i], &game->spectators[i + 1], (game->spectatorsCount - i - 1) * sizeof(PlayerId));
            --game->spectatorsCount;
            break;
        }
    }
    write_client(client->sock, "T'as quitté la partie c'est good\n");
}

void msg_play(Client *listeClients, Client *client, int clientsCount, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    Game *game = NULL;
    if (!findOpponentNameAndGame(&adversaire, clientsCount, listeClients, games, client, &game))
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
    if (game->players[1] == client->player)
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
            for (int j = 0; j < clientsCount; ++j)
            {
                if (listeClients[j].player == game->spectators[i])
                {
                    write_client(listeClients[j].sock, printGame(game, board));
                    break;
                }
            }
        }
        GameStatus result = gameStatus(game, game->currentPlayer);
        Client *p0 = (currentPlayerIndex == 0) ? client : adversaire;
        Client *p1 = (currentPlayerIndex == 1) ? client : adversaire;
        if (result == GAME_WIN_P0)
        {
            write_client(p0->sock, "\nT'as gagné frérot, gg.\n");
            write_client(p1->sock, "\nT'as perdu frérot, laonte.\n");
            Player *winner_p = find_player_by_id(p0->player);
            char spectatorMessage[BUF_SIZE] = {0};
            snprintf(spectatorMessage, sizeof spectatorMessage, "%s a gagné, gg ez pour lui.\n", winner_p ? winner_p->name : "Unknown");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < clientsCount; ++j)
                {
                    if (listeClients[j].player == game->spectators[i])
                    {
                        write_client(listeClients[j].sock, spectatorMessage);
                        break;
                    }
                }
            }
        }
        else if (result == GAME_WIN_P1)
        {
            write_client(p1->sock, "\nT'as gagné frérot, gg.\n");
            write_client(p0->sock, "\nT'as perdu frérot, laonte.\n");
            Player *winner_p = find_player_by_id(p1->player);
            char spectatorMessage[BUF_SIZE] = {0};
            snprintf(spectatorMessage, sizeof spectatorMessage, "%s a gagné, gg ez pour lui.\n", winner_p ? winner_p->name : "Unknown");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < clientsCount; ++j)
                {
                    if (listeClients[j].player == game->spectators[i])
                    {
                        write_client(listeClients[j].sock, spectatorMessage);
                        break;
                    }
                }
            }
        }
        else if (result == GAME_DRAW)
        {
            write_client(p0->sock, "\nEgalité frérot, nul.\n");
            write_client(p1->sock, "\nEgalité frérot, nul.\n");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < clientsCount; ++j)
                {
                    if (listeClients[j].player == game->spectators[i])
                    {
                        write_client(listeClients[j].sock, "\nT'as spectate pour r, ils ont fait égalité ces nuls\n");
                        break;
                    }
                }
            }
        }
        if (result != GAME_ONGOING)
        {
            for (int i = 0; i < games->length; ++i)
            {
                if (&(games->data[i]) == game)
                {
                    vec_splice(games, i, 1);
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

void handle_message(Client *listeClients, Client *client, int clientsCount, char *buffer, Games *games)
{
    // Nettoyage
    buffer[strcspn(buffer, "\r\n")] = 0;
    if (buffer[0] == 0)
        return;

    ClientServerMessage message = {};
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
    case MSG_INFO:
        msg_player_info(client, message.info.pseudo);
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
        Player *p = find_player_by_id(client->player);
        if (p != NULL)
        {
            strncpy(p->bio, message.setbio.bio, sizeof(p->bio) - 1);
            p->bio[sizeof(p->bio) - 1] = 0;
            update_player(p);
            write_client(client->sock, "Bio mise a jour avec succes !\n");
        }
        break;
    }
    default:
        break;
    }
}
