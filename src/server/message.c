#include "server/message.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "server/player.h"
#include "shared/helper.h"
#include "shared/models/clientServer.h"
#include "shared/models/serverClient.h"
#include "shared/vec.h"

static void write_client(SOCKET sock, Server2ClientMessage *buffer)
{
    if (send(sock, buffer, sizeof(Server2ClientMessage), 0) < 0)
    {
        perror("send()");
    }
}

void write_client_status(SOCKET sock, const char *buffer)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_STATUS,
        .status.is_error = false,
    };

    strccpy(raw.status.msg, buffer, sizeof(raw.status.msg));
    write_client(sock, &raw);
}

void write_client_error(SOCKET sock, const char *error)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_STATUS,
        .status.is_error = true,
    };

    strccpy(raw.status.msg, error, sizeof(raw.status.msg));
    write_client(sock, &raw);
}

void write_client_stopstream(SOCKET sock)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_END,
    };

    write_client(sock, &raw);
}

void write_client_players(SOCKET sock, vec_str_t players)
{
    if (players.length == 0)
    {
        Server2ClientMessage raw = {
            .magic = PROTOCOL_MAGIC,
            .kind = MSG_RET_PLAYERS_STREAM,
        };
        write_client(sock, &raw);
        write_client_stopstream(sock);
    }
    for (int i = 0; i < players.length; i += 16)
    {
        Server2ClientMessage raw = {
            .magic = PROTOCOL_MAGIC,
            .kind = MSG_RET_PLAYERS_STREAM,
        };

        for (int j = 0; j < 16 && i + j < players.length; j++)
        {
            strccpy(raw.onlinePlayersSet.names[j], players.data[i + j], sizeof(raw.onlinePlayersSet.names[j]));
        }

        write_client(sock, &raw);
    }

    write_client_stopstream(sock);
}

void write_client_info(SOCKET sock, const char *name, const char *bio, int rank)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_INFO,
    };

    strccpy(raw.playerInfo.name, name, sizeof(raw.playerInfo.name));
    strccpy(raw.playerInfo.bio, bio, sizeof(raw.playerInfo.bio));
    raw.playerInfo.rank = rank;

    write_client(sock, &raw);
}

void write_games(SOCKET sock, Games *games)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_GAMES_STREAM,
    };

    int it = 0;
    int writed = 0;
    int total_in_game = 0;
    while (games->length > it)
    {
        if (games->data[it].status != IN_GAME)
        {
            it++;
            continue;
        }

        total_in_game++;
        Player *p1 = find_player_by_id(games->data[it].players[0]);
        Player *p2 = find_player_by_id(games->data[it].players[1]);

        strccpy(raw.gamesSet.games[writed].p1, p1 ? p1->name : "Joueur 1", sizeof(raw.gamesSet.games[writed].p1));
        strccpy(raw.gamesSet.games[writed].p2, p2 ? p2->name : "Joueur 2", sizeof(raw.gamesSet.games[writed].p2));
        raw.gamesSet.games[writed].id = it;

        writed++;
        if (writed == 8)
        {
            write_client(sock, &raw);
            writed = 0;
            memset(&raw.gamesSet, 0, sizeof(raw.gamesSet));
        }

        it++;
    }

    if (writed > 0 || total_in_game == 0)
    {
        write_client(sock, &raw);
    }

    write_client_stopstream(sock);
}

void write_gameboard(SOCKET sock, Game *game)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_GAME_BOARD,
    };

    for (int i = 0; i < 12; ++i)
    {
        raw.gameBoard.board[i] = game->board[i];
    }

    raw.gameBoard.hands[0] = game->hands[0];
    raw.gameBoard.hands[1] = game->hands[1];

    Player *p1 = find_player_by_id(game->players[0]);
    Player *p2 = find_player_by_id(game->players[1]);
    strccpy(raw.gameBoard.p1, p1 ? p1->name : "Joueur 1", sizeof(raw.gameBoard.p1));
    strccpy(raw.gameBoard.p2, p2 ? p2->name : "Joueur 2", sizeof(raw.gameBoard.p2));
    raw.gameBoard.currentPlayer = game->currentPlayer;

    write_client(sock, &raw);
}

void write_defy_notify(SOCKET sock, const char *challenger)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_DEFY_NOTIFY,
    };

    strccpy(raw.defyNotify.defier, challenger, sizeof(raw.defyNotify.defier));

    write_client(sock, &raw);
}

void write_spectator_notify(SOCKET sock, const char *spectator)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_OBSERVER_NOTIFY,
    };

    strccpy(raw.observerNotify.spectator, spectator, sizeof(raw.observerNotify.spectator));

    write_client(sock, &raw);
}

void write_winner_notify(SOCKET sock, const char *winner, bool is_draw)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_WINNER,
    };

    strccpy(raw.winnerNotify.winner, winner, sizeof(raw.winnerNotify.winner));
    raw.winnerNotify.is_draw = is_draw;

    write_client(sock, &raw);
}

void write_friends(SOCKET sock, vec_str_t friends)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_FRIENDS_STREAM,
    };

    if (friends.length == 0)
    {
        write_client(sock, &raw);
        write_client_stopstream(sock);
        return;
    }
    for (int i = 0; i < friends.length; i += 16)
    {
        memset(&raw.friendsSet, 0, sizeof(raw.friendsSet));
        for (int j = 0; j < 16 && i + j < friends.length; j++)
        {
            strccpy(raw.friendsSet.friends[j], friends.data[i + j], sizeof(raw.friendsSet.friends[j]));
        }

        write_client(sock, &raw);
    }

    write_client_stopstream(sock);
}

void write_game_histories(SOCKET sock, GameHistories *histories)
{
    Server2ClientMessage raw = {
        .magic = PROTOCOL_MAGIC,
        .kind = MSG_RET_HISTORY_STREAM,
    };

    int it = 0;
    int writed = 0;
    while (histories->length > it)
    {
        Player *p1 = find_player_by_id(histories->data[it].p1);
        Player *p2 = find_player_by_id(histories->data[it].p2);
        Player *winner = (histories->data[it].winner == histories->data[it].p1) ? p1 : p2;

        strccpy(raw.gamesSetHistory.games[writed].p1, p1 ? p1->name : "Jsp ki cé", sizeof(raw.gamesSetHistory.games[writed].p1));
        strccpy(raw.gamesSetHistory.games[writed].p2, p2 ? p2->name : "Jsp ki cé", sizeof(raw.gamesSetHistory.games[writed].p2));
        strccpy(raw.gamesSetHistory.games[writed].winner, winner ? winner->name : (histories->data[it].winner == INVALID_PLAYER_ID ? "Egalité" : "Unknown"), sizeof(raw.gamesSetHistory.games[writed].winner));
        raw.gamesSetHistory.games[writed].id = histories->data[it].id;

        writed++;
        if (writed == 8)
        {
            write_client(sock, &raw);
            writed = 0;
            memset(&raw.gamesSetHistory, 0, sizeof(raw.gamesSetHistory));
        }

        it++;
    }

    if (writed > 0)
    {
        write_client(sock, &raw);
    }

    write_client_stopstream(sock);
}
