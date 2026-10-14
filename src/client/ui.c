#include "ui.h"
#include <stdio.h>
#include <string.h>
#include "client/client2.h"
#include "shared/models/clientServer.h"
#include "shared/models/serverClient.h"

static void startPlayerListStream(SOCKET *socket, Server2ClientMessage *starter)
{

    if(starter->leaderboards.is_leaderboard)
    {
        printf("Classement des TITANS GOATED :\n");
    }
    else
    {
        printf("Liste des petits filous connectés :\n");
    }

    while (starter->kind != MSG_RET_END)
    {
        for (int i = 0; i < 16; ++i)
        {
            if (starter->onlinePlayersSet.names[i][0] == '\0')
                break;
            printf("- %s (%i) \n", starter->onlinePlayersSet.names[i],
                starter->onlinePlayersSet.elos[i]);
        }

        receive_message(*socket, starter);
    }
}

static void startGameListStream(SOCKET *socket, Server2ClientMessage *starter)
{
    int count = 0;

    while (starter->kind != MSG_RET_END)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (starter->gamesSet.games[i].p1[0] == '\0')
                break;
            if (count == 0)
            {
                printf("Liste des COMBATS DE TITANS :\n");
            }
            printf("- %s VS %s\n", starter->gamesSet.games[i].p1, starter->gamesSet.games[i].p2);
            count++;
        }

        receive_message(*socket, starter);
    }

    if (count == 0)
    {
        printf("Aucun combat de titan. Lances-en un dcp stp\n");
    }
}

static void displayGameBoard(Server2ClientMessage *msg)
{
    const char *p1Name = msg->gameBoard.p1[0] ? msg->gameBoard.p1 : "Joueur 1";
    const char *p2Name = msg->gameBoard.p2[0] ? msg->gameBoard.p2 : "Joueur 2";
    const char *currName = (msg->gameBoard.currentPlayer == 1) ? p2Name : p1Name;

    printf("Grenier du joueur 1 (%s): %d\n\n\n"
           "\t 0\t\t 1\t\t 2\t\t 3\t\t 4\t\t 5\t\n\n"
           "*-----------------------------------------------------------------------------------------------*\n"
           "|                                                                                               |\n",
           p1Name, msg->gameBoard.hands[0]);

    for (int i = 0; i < 6; i++)
    {
        printf("|\t%2d\t", msg->gameBoard.board[i]);
    }
    printf("|\n|                                                                                               |\n"
           "*-----------------------------------------------------------------------------------------------*\n"
           "|                                                                                               |\n");

    for (int i = 11; i >= 6; --i)
    {
        printf("|\t%2d\t", msg->gameBoard.board[i]);
    }
    printf("|\n|                                                                                               |\n"
           "*-----------------------------------------------------------------------------------------------*\n\n"
           "\t 5\t\t 4\t\t 3\t\t 2\t\t 1\t\t 0\t\n\n"
           "\nGrenier du joueur 2 (%s): %d\n\nAu tour de %s de jouer !\n",
           p2Name, msg->gameBoard.hands[1], currName);
}

static void startFriendsListStream(SOCKET *socket, Server2ClientMessage *starter)
{
    printf("Liste des amis :\n");
    int count = 0;

    while (starter->kind != MSG_RET_END)
    {
        for (int i = 0; i < 16; ++i)
        {
            if (starter->friendsSet.friends[i][0] == '\0')
                break;
            printf("- %s\n", starter->friendsSet.friends[i]);
            count++;
        }

        receive_message(*socket, starter);
    }

    if (count == 0)
    {
        printf("(Aucun ami)\n");
    }
}

static void startGameHistoryStream(SOCKET *socket, Server2ClientMessage *starter)
{
    while (starter->kind != MSG_RET_END)
    {
        for (int i = 0; i < 8; ++i)
        {
            if (starter->gamesSetHistory.games[i].p1[0] == '\0')
                break;
            printf("\n================================\n");
            printf("\t\tGame n°%d\n", starter->gamesSetHistory.games[i].id);
            printf("%s VS %s\n", starter->gamesSetHistory.games[i].p1, starter->gamesSetHistory.games[i].p2);
            printf("LE GOAT (gagnant) : %s\n", starter->gamesSetHistory.games[i].winner[0] ? starter->gamesSetHistory.games[i].winner : "Inconnu");
            printf("================================\n");
        }

        receive_message(*socket, starter);
    }
}

void display_message(SOCKET *initiator, Server2ClientMessage *msg)
{
    if (msg->magic != PROTOCOL_MAGIC)
    {
        printf("client side error: Invalid magic number\n");
        return;
    }

    switch (msg->kind)
    {
    case MSG_RET_STATUS:
    {
        if (msg->status.is_error)
            printf("Error: %s", msg->status.msg);
        else
            printf("%s", msg->status.msg);

        size_t len = strlen(msg->status.msg);
        if (len == 0 || msg->status.msg[len - 1] != '\n')
            printf("\n");

        break;
    }
    case MSG_RET_PLAYERS_STREAM:
    {
        startPlayerListStream(initiator, msg);
        break;
    }
    case MSG_RET_INFO:
    {
        printf("Information sur le joueur %s :\n"
               "  Bio: %s\n"
               "  Rank: %d\n",
               msg->playerInfo.name, msg->playerInfo.bio, msg->playerInfo.rank);
        break;
    }
    case MSG_RET_GAMES_STREAM:
    {
        startGameListStream(initiator, msg);
        break;
    }
    case MSG_RET_GAME_BOARD:
    {
        displayGameBoard(msg);
        break;
    }
    case MSG_RET_DEFY_NOTIFY:
    {
        printf("1V1 NO RE CONTRE %s ?\n", msg->defyNotify.defier);
        break;
    }
    case MSG_RET_OBSERVER_NOTIFY:
    {
        printf("%s se cache dans les buissons pour vous observer...\n", msg->observerNotify.spectator);
        break;
    }
    case MSG_RET_WINNER:
    {
        if (msg->winnerNotify.is_draw)
        {
            printf("\nT'as spectate pour r, ils ont fait égalité ces nuls\n");
        }
        else
        {
            printf("%s a gagné, gg ez pour lui.\n", msg->winnerNotify.winner);
        }
        break;
    }
    case MSG_RET_FRIENDS_STREAM:
    {
        startFriendsListStream(initiator, msg);
        break;
    }
    case MSG_RET_HISTORY_STREAM:
    {
        startGameHistoryStream(initiator, msg);
        break;
    }
    case MSG_RET_END:
        break;
    default:
        printf("client side error: unrecognized message kind: %d\n", msg->kind);
        break;
    }
}
