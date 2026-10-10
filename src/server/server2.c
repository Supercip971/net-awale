#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "server/commands.h"
#include "server/message.h"
#include "server/models/games.h"
#include "server/player.h"
#include "server/server2.h"
#include "shared/game.h"
#include "shared/vec.h"

void removeGamesOf(Games *games, PlayerId player)
{
    for (int i = games->length - 1; i >= 0; --i)
    {
        if (games->data[i].players[0] == player ||
            games->data[i].players[1] == player)
        {
            vec_splice(games, i, 1);
        }
    }
    // FIXME: cette fonction est trop lourde, rajouter un removeSpectatorOf
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

int check_name_exist(Client *listeClients, int clientCount, const char *name)
{
    for (int i = 0; i < clientCount; ++i)
    {
        Player *p = find_player_by_id(listeClients[i].player);
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
}

void end(void)
{
#ifdef WIN32
    WSACleanup();
#endif
}

void app(void)
{
    players_db_init();
    SOCKET sock = init_connection();
    char buffer[BUF_SIZE];
    /* the index for the array */
    int actual = 0;
    int max = sock;
    /* an array for all clients */
    Client clients[MAX_CLIENTS];
    Games games = {0};
    vec_init(&games);

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
        for (i = 0; i < actual; i++)
        {
            FD_SET(clients[i].sock, &rdfs);
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

            if (actual >= MAX_CLIENTS)
            {
                printf("Serveur plein CLAMERD");
                write_client(csock, "Serveur plein.\n");
                closesocket(csock);
                continue;
            }

            /* after connecting the client sends its name */
            if (read_client(csock, buffer) <= 0)
            {
                /* disconnected */
                closesocket(csock);
                continue;
            }

            // On enlève les \r et \n des pseudos
            buffer[strcspn(buffer, "\r\n")] = 0;
            if (buffer[0] == 0 || strlen(buffer) >= MAX_USERNAME_LENGTH || check_name_exist(clients, actual, buffer))
            {
                write_client(csock, "Le pseudo existe déjà. Sois original stp\n");
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

            Client c = {.sock = csock, .player = pid};
            clients[actual] = c;
            actual++;
        }
        else
        {
            for (i = 0; i < actual; i++)
            {
                /* a client is talking */
                if (FD_ISSET(clients[i].sock, &rdfs))
                {
                    Client client = clients[i];
                    int c = read_client(clients[i].sock, buffer);
                    /* client disconnected */
                    if (c == 0)
                    {
                        closesocket(clients[i].sock);
                        remove_client(clients, i, &actual);

                        max = sock;
                        for (int j = 0; j < actual; j++)
                            if (clients[j].sock > max)
                                max = clients[j].sock;

                        Player *disconnected_player = find_player_by_id(client.player);
                        const char *name = disconnected_player ? disconnected_player->name : "Unknown";
                        strncpy(buffer, name, BUF_SIZE - 1);
                        buffer[BUF_SIZE - 1] = 0;
                        strncat(buffer, " disconnected !", BUF_SIZE - strlen(buffer) - 1);
                        removeGamesOf(&games, client.player);
                        send_message_to_all_clients(clients, client, actual, buffer, 1);
                    }
                    else
                    {
                        handle_message(clients, &client, actual, buffer, &games);
                    }
                    break;
                }
            }
        }
    }

    vec_deinit(&games);
    players_db_deinit();
    clear_clients(clients, actual);
    end_connection(sock);
}

void clear_clients(Client *clients, int actual)
{
    for (int i = 0; i < actual; i++)
    {
        closesocket(clients[i].sock);
    }
}

void remove_client(Client *clients, int to_remove, int *actual)
{
    /* we remove the client in the array */
    memmove(clients + to_remove, clients + to_remove + 1, (*actual - to_remove - 1) * sizeof(Client));
    /* number client - 1 */
    (*actual)--;
}

void send_message_to_all_clients(Client *clients, Client sender, int actual, const char *buffer, char from_server)
{
    char message[BUF_SIZE];
    for (int i = 0; i < actual; i++)
    {
        message[0] = 0; // reset str
        /* we don't send message to the sender */
        if (sender.sock != clients[i].sock)
        {
            size_t offset = 0;
            if (from_server == 0)
            {
                Player *p = find_player_by_id(sender.player);
                const char *name = p ? p->name : "Unknown";
                offset += snprintf(message, sizeof message - offset, "%s : ", name);
            }

            offset += snprintf(message + offset, sizeof message - offset, "%s", buffer);
            write_client(clients[i].sock, message);
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
    int n = 0;

    if ((n = recv(sock, buffer, BUF_SIZE - 1, 0)) < 0)
    {
        perror("recv()");
        /* if recv error we disonnect the client */
        n = 0;
    }

    buffer[n] = 0;

    return n;
}

int main(void)
{
    init();

    app();

    end();

    return EXIT_SUCCESS;
}
