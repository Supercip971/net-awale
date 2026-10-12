#include "server/commands.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server/client2.h"
#include "server/game.h"
#include "server/history.h"
#include "server/message.h"
#include "server/player.h"
#include "server/server2.h"
#include "shared/models/clientServer.h"
#include "shared/vec.h"
#include "shared/writer.h"

// TODO: Refactor with a map player: socket (avoiding n loops)

Client *checkOpponent(char pseudo[MAX_USERNAME_LENGTH], Clients *connections, Client *client)
{
    Player *player = find_player_by_name(pseudo);
    Client *res = NULL;
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

    for (int i = 0; i < connections->length; ++i)
    {
        if (connections->data[i].player == player->user_id)
        {
            res = &connections->data[i];
            return res;
        }
    }

    write_client(client->sock, "Il n'est pas en ligne\n");
    return 0;
}

int findOpponentNameAndGame(Client **adversaire, Clients *connections, Games *games, Client *client, Game **res)
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

    for (int i = 0; i < connections->length; ++i)
    {
        if (connections->data[i].player == opponent)
        {
            *adversaire = &connections->data[i];
            break;
        }
    }
    if (*adversaire == NULL || (*adversaire)->sock == client->sock)
    {
        return 0;
    }
    return 1;
}

void msg_players(Clients *connections, Client *client)
{
    Writer wstr = {};
    w_init(&wstr);
    w_write(&wstr, "Liste des petits filous connectés :\n");
    for (int i = 0; i < connections->length; ++i)
    {
        Player *player = find_player_by_id(connections->data[i].player);
        const char *name = player ? player->name : "Anonyme(erroeur)";
        w_write(&wstr, "- %s\n", name);
    }

    write_client(client->sock, w_view(&wstr));
    w_deinit(&wstr);
}

void msg_player_info(Client *client, const char *pname)
{
    Writer wstr = {};
    w_init(&wstr);
    Player *player = find_player_by_name(pname);
    if (player)
    {
        w_write(&wstr, "Information sur le joueur %s :\n"
                       "  Bio: %s\n"
                       "  Rank: %d\n",
                player->name, player->bio, player->rank);
        write_client(client->sock, w_view(&wstr));
        w_deinit(&wstr);
        return;
    }
    write_client(client->sock, "Joueur non trouvé.\n");
}

void msg_games(Games *games, Client *client)
{

    unsigned int runningGamesCount = 0;

    Writer wstr = {};
    w_init(&wstr);
    w_write(&wstr, "Liste des COMBATS DE TITANS :\n");
    for (int i = 0; i < games->length; ++i)
    {
        if (games->data[i].status != IN_GAME)
            continue;
        w_write(&wstr, "- ");
        displayGame(&(games->data[i]), &wstr);
        w_write(&wstr, "\n");

        ++runningGamesCount;
    }
    if (!runningGamesCount)
    {
        write_client(client->sock, "Aucun combat de titan. Lances-en un dcp stp\n");
        return;
    }

    write_client(client->sock, w_view(&wstr));

    w_deinit(&wstr);
}

void msg_message(Clients *connections, Client *client, ClientServerMessage *message)
{
    send_message_to_all_clients(connections, *client, message->message.message, 0);
}

void msg_accept_defy(Clients *connections, Client *client, Games *games, ClientServerMessage *message)
{
    Client *adversaire = checkOpponent(message->acceptDefy.pseudo, connections, client);
    if (!adversaire)
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

void msg_defy(Clients *connections, Client *client, Games *games, ClientServerMessage *message)
{
    Client *adversaire = checkOpponent(message->defy.pseudo, connections, client);
    if (!adversaire)
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
            msg_accept_defy(connections, client, games, message);
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

    Writer wstr = {};

    w_init(&wstr);
    w_write(&wstr, "1V1 NO RE CONTRE %s ?\n", client_p ? client_p->name : "Unknown");

    write_client(adversaire->sock, w_view(&wstr));
    w_deinit(&wstr);
}

void msg_decline_defy(Clients *connections, Client *client, Games *games, ClientServerMessage *message)
{
    Client *adversaire = checkOpponent(message->declineDefy.pseudo, connections, client);
    if (!adversaire)
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

    Writer wstr = {};
    w_init(&wstr);
    w_write(&wstr, "déso, %s a trop peur, il a refusé\n", client_p ? client_p->name : "Unknown");
    write_client(adversaire->sock, w_view(&wstr));
    w_deinit(&wstr);
}

int get_players_clients(Clients *connections, Game *game, Client **opponent1, Client **opponent2)
{
    for (int i = 0; i < connections->length; ++i)
    {
        if (connections->data[i].player == game->players[0])
        {
            *opponent1 = &connections->data[i];
        }
        else if (connections->data[i].player == game->players[1])
        {
            *opponent2 = &connections->data[i];
        }
    }
    if (*opponent1 == NULL || *opponent2 == NULL)
    {
        return 0;
    }
    return 1;
}

void msg_spec(Clients *connections, Client *client, Games *games, ClientServerMessage *message)
{
    Client *adversaire1 = checkOpponent(message->spec.pseudo, connections, client);
    if (!adversaire1)
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

    Client *p0 = NULL;
    Client *p1 = NULL;
    if (!get_players_clients(connections, game, &p0, &p1))
    {
        write_client(client->sock, "Les joueurs existent pas frr (ils ont dû se déco entre temps, réessaie)\n");
        return;
    }

    if (game->private)
    {
        Player *player0 = find_player_by_id(p0->player);
        Player *player1 = find_player_by_id(p1->player);
        if (player0 == NULL || player1 == NULL)
        {
            write_client(client->sock, "Erreur, joueurs introuvables\n");
            return;
        }
        if (!has_friend(player0, client->player) && !has_friend(player1, client->player))
        {
            write_client(client->sock, "NANANANAN C'EST PRIVE.\n");
            return;
        }
    }

    game->spectators[game->spectatorsCount++] = client->player;

    Player *client_p = find_player_by_id(client->player);

    Writer wstr = {};
    w_init(&wstr);

    w_write(&wstr, "%s se cache dans les buissons pour vous observer...\n", client_p ? client_p->name : "Un inconnu");
    write_client(p0->sock, w_view(&wstr));
    write_client(p1->sock, w_view(&wstr));
    w_deinit(&wstr);
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

void msg_play(Clients *connections, Client *client, Games *games, ClientServerMessage *message)
{
    Client *adversaire = NULL;
    Game *game = NULL;
    if (!findOpponentNameAndGame(&adversaire, connections, games, client, &game))
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
        // send to client new board
        write_client(adversaire->sock, printGame(game, board));
        write_client(client->sock, printGame(game, board));

        for (int i = 0; i < game->spectatorsCount; ++i)
        {
            for (int j = 0; j < connections->length; ++j)
            {
                if (connections->data[j].player == game->spectators[i])
                {
                    write_client(connections->data[j].sock, printGame(game, board));
                    break;
                }
            }
        }

        // update game status
        vec_push(&game->turns, message->play.hole);

        GameStatus result = gameStatus(game, game->currentPlayer);
        Client *p0 = (currentPlayerIndex == 0) ? client : adversaire;
        Client *p1 = (currentPlayerIndex == 1) ? client : adversaire;
        if (result == GAME_WIN_P0)
        {
            gameHistoryGameEnded(game, 0);

            write_client(p0->sock, "\nT'as gagné frérot, gg.\n");
            write_client(p1->sock, "\nT'as perdu frérot, laonte.\n");
            Player *winner_p = find_player_by_id(p0->player);
            Writer wstr = {};
            w_init(&wstr);
            w_write(&wstr, "%s a gagné, gg ez pour lui.\n", winner_p ? winner_p->name : "Unknown");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < connections->length; ++j)
                {
                    Client *spectator = &connections->data[j];
                    if (spectator->player == game->spectators[i])
                    {
                        write_client(spectator->sock, w_view(&wstr));
                        break;
                    }
                }
            }
            w_deinit(&wstr);
        }
        else if (result == GAME_WIN_P1)
        {
            gameHistoryGameEnded(game, 1);

            write_client(p1->sock, "\nT'as gagné frérot, gg.\n");
            write_client(p0->sock, "\nT'as perdu frérot, laonte.\n");
            Player *winner_p = find_player_by_id(p1->player);
            Writer wstr = {};
            w_init(&wstr);
            w_write(&wstr, "%s a gagné, gg ez pour lui.\n", winner_p ? winner_p->name : "Unknown");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < connections->length; ++j)
                {
                    Client *spectator = &connections->data[j];
                    if (spectator->player == game->spectators[i])
                    {
                        write_client(spectator->sock, w_view(&wstr));
                        break;
                    }
                }
            }
            w_deinit(&wstr);
        }
        else if (result == GAME_DRAW)
        {
            write_client(p0->sock, "\nEgalité frérot, nul.\n");
            write_client(p1->sock, "\nEgalité frérot, nul.\n");
            for (int i = 0; i < game->spectatorsCount; ++i)
            {
                for (int j = 0; j < connections->length; ++j)
                {
                    Client *spectator = &connections->data[j];
                    if (spectator->player == game->spectators[i])
                    {
                        write_client(spectator->sock, "\nT'as spectate pour r, ils ont fait égalité ces nuls\n");
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
                    // not deinit it is handled in game saving
                    // vec_deinit(&games->data[i].turns);
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

void msg_set_private(Client *client, Games *games, int private, Clients *connections)
{
    Game *game = find_game(games, client->player);
    if (game == NULL)
    {
        write_client(client->sock, "T'es même pas en game frérot\n");
        return;
    }
    set_game_private(game, private, connections);
}

void msg_friends(Client *client)
{
    Player *current = find_player_by_id(client->player);

    // empty
    Writer wstr = {};
    w_init(&wstr);

    w_write(&wstr, "Liste des amis :\n");
    for (int i = 0; i < current->friends.length; ++i)
    {

        Player *friend = find_player_by_id(current->friends.data[i]);
        const char *name = friend ? friend->name : "Anonyme(erroeur)";
        w_write(&wstr, "- %s\n", name);
    }

    write_client(client->sock, w_view(&wstr));
    w_deinit(&wstr);
    return;
}

void msg_addFriend(Client *client, ClientServerMessage *msg)
{
    Player *current = find_player_by_id(client->player);

    Player *new_friend = find_player_by_name(msg->addFriend.pseudo);

    if (new_friend == NULL)
    {
        write_client(client->sock, "error: friend non existant");
        return;
    }

    vec_push(&current->friends, new_friend->user_id);
    write_client(client->sock, "ami ajouté !");
    return;
}

void printGameHistory(GameHistory *gameHistory, void *ctx)
{
    Writer *writer = ctx;
    Player *player1 = find_player_by_id(gameHistory->p1);
    Player *player2 = find_player_by_id(gameHistory->p2);
    Player *winner = gameHistory->winner == gameHistory->p1 ? player1 : player2;

    const char *n1 = player1 ? player1->name : "Jsp ki cé";
    const char *n2 = player2 ? player2->name : "Jsp ki cé";
    const char *nw = winner ? winner->name : "Jsp ki cé";

    w_write(writer, "\n================================\n");
    w_write(writer, "\t\tGame n°%d\n", (int)gameHistory->id);
    w_write(writer, "%s VS %s\n", n1, n2);
    w_write(writer, "LE GOAT (gagnant) : %s\n", nw);
    w_write(writer, "================================\n");
}

void msg_history(Client *client)
{
    Writer wstr = {};
    w_init(&wstr);
    int count = gameHistoryForEachPlayer(client->player, printGameHistory, &wstr);
    if (!count)
    {
        write_client(client->sock, "Aucune partie jouée.");
        return;
    }
    write_client(client->sock, w_view(&wstr));
    w_deinit(&wstr);
}

// startIndex = 1 si pas commencé
// continueToEnd = booléen
void doReplay(Client *client, int continueToEnd)
{
    GameHistory *gameHistory = getGameHistoryById(client->currentGameHistoryId);
    if (gameHistory == NULL)
    {
        write_client(client->sock, "Cette partie n'existe pas frérot.\n");
        return;
    }
    if (client->currentReplayStep >= gameHistory->turns.length)
    {
        write_client(client->sock, "Replay terminé frérot.\n");
        return;
    }
    Game game;
    gameInit(&game);
    game.players[0] = gameHistory->p1;
    game.players[1] = gameHistory->p2;
    game.started = (gameHistory->started == gameHistory->p2) ? 1 : 0;
    game.currentPlayer = game.started;

    for (int i = 0; i < client->currentReplayStep; i++)
    {
        int captured = 0;
        play(&game, game.currentPlayer, gameHistory->turns.data[i], &captured);
        game.currentPlayer = game.currentPlayer ? 0 : 1;
    }

    for (int i = client->currentReplayStep; i < gameHistory->turns.length; i++)
    {
        int captured = 0;
        if (!play(&game, game.currentPlayer, gameHistory->turns.data[i], &captured))
        {
            write_client(client->sock, "Replay corrompu c'est mort déso\n");
            break;
        }
        game.currentPlayer = game.currentPlayer ? 0 : 1;
        client->currentReplayStep = i + 1;
        GameStatus result = gameStatus(&game, game.currentPlayer);

        char board[BUF_SIZE] = {0};
        write_client(client->sock, printGame(&game, board));
        if (result == GAME_WIN_P0 || result == GAME_WIN_P1)
        {
            int winnerIndex = (result == GAME_WIN_P1) ? 1 : 0;
            Player *winner_p = find_player_by_id(game.players[winnerIndex]);
            Writer wstr = {};
            w_init(&wstr);
            w_write(&wstr, "%s a gagné, gg ez pour lui.\n", winner_p ? winner_p->name : "Unknown");
            write_client(client->sock, w_view(&wstr));
            w_deinit(&wstr);
            break;
        }
        else if (result == GAME_DRAW)
        {
            write_client(client->sock, "\nT'as spectate pour r, ils ont fait égalité ces nuls\n");
            break;
        }
        if (!continueToEnd)
            break;
    }
    vec_deinit(&game.turns);
}

void msg_replay_game(Client *client, GameHistoryId gameHistoryId)
{
    if (getGameHistoryById(gameHistoryId) == NULL)
    {
        write_client(client->sock, "Cette partie n'existe pas frérot.\n");
        return;
    }
    client->currentReplayStep = 0;
    client->currentGameHistoryId = gameHistoryId;
    doReplay(client, 0);
}

void handle_message(Clients *connections, Client *client, ClientServerMessage *message, Games *games)
{
    if (message->protocol_magic != PROTOCOL_MAGIC)
    {
        write_client(client->sock, "Invalid magic number\n");
        return;
    }

    // Traitement des messages (en fonction de leur kind)
    switch (message->kind)
    {
    case MSG_PLAYERS:
        msg_players(connections, client);
        break;
    case MSG_INFO:
        msg_player_info(client, message->info.pseudo);
        break;
    case MSG_GAMES:
        msg_games(games, client);
        break;
    case MSG_MESSAGE:
        msg_message(connections, client, message);
        break;
    case MSG_DEFY:
        msg_defy(connections, client, games, message);
        break;
    case MSG_DECLINE_DEFY:
        msg_decline_defy(connections, client, games, message);
        break;
    case MSG_ACCEPT_DEFY:
        msg_accept_defy(connections, client, games, message);
        break;
    case MSG_PLAY:
        msg_play(connections, client, games, message);
        break;
    case MSG_SPEC:
        msg_spec(connections, client, games, message);
        break;
    case MSG_STOP_SPEC:
        msg_stop_spec(client, games);
        break;
    case MSG_FRIENDS:
        msg_friends(client);
        break;
    case MSG_ADDFRIEND:
        msg_addFriend(client, message);
        break;
    case MSG_SETBIO:
    {
        Player *p = find_player_by_id(client->player);
        if (p != NULL)
        {
            strncpy(p->bio, message->setbio.bio, sizeof(p->bio) - 1);
            p->bio[sizeof(p->bio) - 1] = 0;
            update_player(p);
            write_client(client->sock, "Bio mise a jour avec succes !\n");
        }
        break;
    }
    case MSG_SET_PRIVATE:
        msg_set_private(client, games, message->setPrivate.private, connections);
        break;
    case MSG_HISTORY:
        msg_history(client);
        break;
    case MSG_REPLAY_GAME:
        msg_replay_game(client, message->replayGame.id);
        break;
    case MSG_STEP_REPLAY:
        doReplay(client, 0);
        break;
    case MSG_CONTINUE_REPLAY:
        doReplay(client, 1);
        break;
    default:
        break;
    }
}
