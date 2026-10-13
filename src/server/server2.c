#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <time.h>
#include "server/client2.h"
#include "server/commands.h"
#include "server/history.h"
#include "server/message.h"
#include "server/game.h"
#include "server/player.h"
#include "server/server2.h"
#include "shared/game.h"
#include "shared/models/clientServer.h"
#include "shared/vec.h"

void removeGamesOf(Games *games, Clients *clients, PlayerId player)
{
    for (int i = games->length - 1; i >= 0; --i)
    {
        Game *g = &games->data[i];
        int disconnecting_player = -1;
        if (g->players[0] == player)
        {
            disconnecting_player = 0;
        }
        else if (g->players[1] == player)
        {
            disconnecting_player = 1;
        }
        else
        {
            continue;
        }

        if (g->status == WAITING)
        {
            PlayerId other_player = (disconnecting_player == 0) ? g->players[1] : g->players[0];
            if (clients != NULL)
            {
                for (int j = 0; j < clients->length; ++j)
                {
                    if (clients->data[j].player == other_player)
                    {
                        write_client_error(clients->data[j].sock, "Votre adversaire s'est déconnecté, le défi a été annulé.\n");
                        break;
                    }
                }
            }
            vec_deinit(&g->turns);
            vec_splice(games, i, 1);
        }
        else if (g->status == IN_GAME)
        {
            // Soft giving up: preserve the game so the player can reconnect,
            // or the adversary can type 'giveup' to claim victory.
            g->giving_up[disconnecting_player] = 1;
        }
    }
    for (int i = 0; i < games->length; ++i)
    {
        Game *g = &games->data[i];
        for (int j = 0; j < g->spectatorsCount; ++j)
        {
            if (g->spectators[j] == player)
            {
                memmove(&g->spectators[j], &g->spectators[j + 1], (g->spectatorsCount - j - 1) * sizeof(PlayerId));
                --g->spectatorsCount;
                break;
            }
        }
    }
}

int check_name_exist(Clients *listeClients, const char *name)
{
    for (int i = 0; i < listeClients->length; ++i)
    {
        Player *p = find_player_by_id(listeClients->data[i].player);
        if (p && strcmp(p->name, name) == 0)
        {
            return 1;
        }
    }
    return 0;
}

void init(void)
{
#ifdef WIN32
    WSADATA wsa;
    int err = WSAStartup(MAKEWORD(2, 2), &wsa);
    if (err < 0)
    {
        puts("WSAStartup failed !");
        exit(EXIT_FAILURE);
    }
#endif

    srand((unsigned int)time(NULL));
}

void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

void app(void)
{
    signal(SIGPIPE, SIG_IGN);
    players_db_init();
    gameHistoryInit();
    SOCKET sock = init_connection();
    char buffer[sizeof(ClientServerMessage)];
    int max = sock;
    /* an array for all clients */
    Games games = {0};
    vec_init(&games);

    Clients clients = {0};
    vec_init(&clients);
    fd_set rdfs;

    printf("Server started on port %d\n", PORT);
    while (1)
    {
        int i = 0;
        FD_ZERO(&rdfs);

        /* add STDIN_FILENO */
        FD_SET(STDIN_FILENO, &rdfs);

        /* add the connection socket */
        FD_SET(sock, &rdfs);

        /* add socket of each client */
        for (i = 0; i < clients.length; i++)
        {
            FD_SET(clients.data[i].sock, &rdfs);
        }

        if (select(max + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        /* something from standard input : i.e keyboard */
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            /* stop process when type on keyboard */
            break;
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            /* new client */
            SOCKADDR_IN csin = {0};
            socklen_t sinsize = sizeof csin;
            int csock = accept(sock, (SOCKADDR *)&csin, &sinsize);
            if (csock == SOCKET_ERROR)
            {
                perror("accept()");
                continue;
            }

            if (clients.length >= MAX_CLIENTS)
            {
                printf("Serveur plein CLAMERD\n");
                write_client_error(csock, "Serveur plein.\n");
                closesocket(csock);
                continue;
            }

            /* after connecting the client sends its name */
            int n = recv(csock, buffer, MAX_USERNAME_LENGTH - 1, 0);
            if (n <= 0)
            {
                /* disconnected */
                closesocket(csock);
                continue;
            }
            buffer[n] = 0;

            // On enlève les \r et \n des pseudos
            buffer[strcspn(buffer, "\r\n")] = 0;
            if (buffer[0] == 0 || strlen(buffer) >= MAX_USERNAME_LENGTH || check_name_exist(&clients, buffer))
            {
                write_client_error(csock, "Le pseudo existe déjà. Sois original stp\n");
                closesocket(csock);
                continue;
            }

            /* what is the new maximum fd ? */
            max = csock > max ? csock : max;

            FD_SET(csock, &rdfs);

            Player *existing = find_player_by_name(buffer);
            PlayerId pid;
            if (existing != NULL)
            {
                pid = existing->user_id;
            }
            else
            {
                Player new_p = {0};
                memcpy(new_p.name, buffer, strlen(buffer));

                pid = create_player(&new_p);
            }

            Client c = {.sock = csock, .player = pid, .currentReplayStep = -1, .currentGameHistoryId = -1};
            vec_push(&clients, c);

            Game *runningGame = find_game(&games, pid);
            if (runningGame != NULL)
            {
                int p = (runningGame->players[1] == pid) ? 1 : 0;
                runningGame->giving_up[p] = 0;
                write_client_status(csock, "Tu as rejoint ta partie en cours !\n");
                write_gameboard(csock, runningGame);

                PlayerId opponentId = runningGame->players[1 - p];
                for (int j = 0; j < clients.length; ++j)
                {
                    if (clients.data[j].player == opponentId)
                    {
                        write_client_status(clients.data[j].sock, "Votre adversaire s'est reconnecté !\n");
                        write_gameboard(clients.data[j].sock, runningGame);
                        break;
                    }
                }
            }
        }
        else
        {
            for (i = 0; i < clients.length; i++)
            {
                /* a client is talking */
                if (FD_ISSET(clients.data[i].sock, &rdfs))
                {
                    Client client = clients.data[i];
                    int c = read_client(client.sock, buffer);

                    /* client disconnected */
                    if (c == 0)
                    {
                        closesocket(client.sock);
                        remove_client(&clients, i);

                        max = sock;
                        for (int j = 0; j < clients.length; j++)
                            if (clients.data[j].sock > max)
                                max = clients.data[j].sock;

                        Player *disconnected_player = find_player_by_id(client.player);
                        const char *name = disconnected_player ? disconnected_player->name : "Unknown";
                        strncpy(buffer, name, BUF_SIZE - 1);
                        buffer[BUF_SIZE - 1] = 0;
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        signal_disconnect(&clients, &client, &games);
                        removeGamesOf(&games, &clients, client.player);
                        send_message_to_all_clients(&clients, client, buffer, 1);
                    }
                    else
                    {
                        handle_message(&clients, &clients.data[i], (ClientServerMessage *)buffer, &games);
                    }
                    break;
                }
            }
        }
    }

    vec_deinit(&games);
    players_db_deinit();
    gameHistoryDeinit();
    clear_clients(&clients);
    end_connection(sock);
}

void clear_clients(Clients *clients)
{
    for (int i = 0; i < clients->length; i++)
    {
        closesocket(clients->data[i].sock);
    }
}

void remove_client(Clients *clients, int to_remove)
{
    /* we remove the client in the array */
    vec_splice(clients, to_remove, 1);
}

void send_message_to_all_clients(Clients *clients, Client sender, const char *buffer, char from_server)
{
    char message[BUF_SIZE];
    for (int i = 0; i < clients->length; i++)
    {
        message[0] = 0; // reset str
        /* we don't send message to the sender */
        auto client = clients->data[i];
        if (sender.sock != client.sock)
        {
            size_t offset = 0;
            if (from_server == 0)
            {
                Player *p = find_player_by_id(sender.player);
                const char *name = p ? p->name : "Unknown";
                offset += snprintf(message, sizeof message - offset, "%s : ", name);
            }

            offset += snprintf(message + offset, sizeof message - offset, "%s", buffer);
            write_client_status(client.sock, message);
        }
    }
}

int init_connection(void)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    int opt = 1;
    setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof opt);

    sin.sin_addr.s_addr = htonl(INADDR_ANY);
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (bind(sock, (SOCKADDR *)&sin, sizeof sin) == SOCKET_ERROR)
    {
        perror("bind()");
        exit(errno);
    }

    if (listen(sock, MAX_CLIENTS) == SOCKET_ERROR)
    {
        perror("listen()");
        exit(errno);
    }

    return sock;
}

void end_connection(int sock)
{
    closesocket(sock);
}

int read_client(SOCKET sock, char *buffer)
{
    size_t total = 0;
    while (total < sizeof(ClientServerMessage))
    {
        int n = recv(sock, buffer + total, sizeof(ClientServerMessage) - total, 0);
        if (n < 0)
        {
            perror("recv()");
            /* if recv error we disconnect the client */
            return 0;
        }
        if (n == 0)
        {
            return 0;
        }
        total += n;
    }

    return total;
}

int main(void)
{
    init();

    app();

    end();

    return EXIT_SUCCESS;
}
