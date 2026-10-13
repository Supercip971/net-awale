#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "client/client2.h"
#include "client/help.h"
#include "client/parser.h"
#include "client/ui.h"
#include "server/common.h"
#include "shared/models/clientServer.h"
#include "shared/models/serverClient.h"

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

void onUserInput(SOCKET sock, const char *buffer)
{

    ClientServerMessage *msg = encode_message(buffer);
    if (msg == NULL)
    {
        printf("invalid command, use `help` to show available commands\n$ ");
        fflush(stdout);
        return;
    }
    else if (msg->kind == MSG_CLIENTSIDE_HELP)
    {
        showHelp();
        free(msg);
        return;
    }
    if (send(sock, msg, sizeof(*msg), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
    free(msg);
}

void receive_message(SOCKET sock, Server2ClientMessage *msg)
{
    int n = read_server(sock, (char *)msg, sizeof(Server2ClientMessage));
    if (n <= 0)
    {
        printf("Server disconnected !\n");
        msg->kind = MSG_RET_END;
    }
}

void app(const char *address, const char *name)
{
    SOCKET sock = init_connection(address);
    size_t buffer_size = BUF_SIZE;
    if (sizeof(Server2ClientMessage) > buffer_size)
    {
        buffer_size = sizeof(Server2ClientMessage);
    }
    if (sizeof(ClientServerMessage) > buffer_size)
    {
        buffer_size = sizeof(ClientServerMessage);
    }
    char *buffer = malloc(buffer_size);

    fd_set rdfs;

    /* send our name */
    write_server(sock, name);

    printf("Connected to server %s:%d as %s\n", address, PORT, name);
    printf("\n$ ");
    fflush(stdout); // sur ma machine le $ n'est pas affiché
    while (1)
    {
        FD_ZERO(&rdfs);

        /* add STDIN_FILENO */
        FD_SET(STDIN_FILENO, &rdfs);

        /* add the socket */
        FD_SET(sock, &rdfs);

        if (select(sock + 1, &rdfs, NULL, NULL, NULL) == -1)
        {
            perror("select()");
            exit(errno);
        }

        /* something from standard input : i.e keyboard */
        if (FD_ISSET(STDIN_FILENO, &rdfs))
        {
            fgets(buffer, buffer_size - 1, stdin);
            {
                char *p = NULL;
                p = strstr(buffer, "\n");
                if (p != NULL)
                {
                    *p = 0;
                }
                else
                {
                    /* fclean */
                    buffer[buffer_size - 1] = 0;
                }
            }
            onUserInput(sock, buffer);
            fflush(stdout); // sur ma machine le $ n'est pas affiché à cause de \n
        }
        else if (FD_ISSET(sock, &rdfs))
        {
            int n = read_server(sock, buffer, sizeof(Server2ClientMessage));
            /* server down */
            if (n <= 0)
            {
                printf("Server disconnected !\n");
                break;
            }

            display_message(&sock, (Server2ClientMessage *)buffer);

            printf("\n$ ");
            fflush(stdout); // sur ma machine le $ n'est pas affiché à cause de \n
        }
    }

    free(buffer);
    end_connection(sock);
}

int init_connection(const char *address)
{
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN sin = {0};
    struct hostent *hostinfo;

    if (sock == INVALID_SOCKET)
    {
        perror("socket()");
        exit(errno);
    }

    hostinfo = gethostbyname(address);
    if (hostinfo == NULL)
    {
        fprintf(stderr, "Unknown host %s.\n", address);
        exit(EXIT_FAILURE);
    }

    sin.sin_addr = *(IN_ADDR *)hostinfo->h_addr;
    sin.sin_port = htons(PORT);
    sin.sin_family = AF_INET;

    if (connect(sock, (SOCKADDR *)&sin, sizeof(SOCKADDR)) == SOCKET_ERROR)
    {
        perror("connect()");
        exit(errno);
    }

    return sock;
}

void end_connection(int sock)
{
    closesocket(sock);
}

int read_server(SOCKET sock, char *buffer, size_t len)
{
    size_t total = 0;
    while (total < len)
    {
        int n = recv(sock, buffer + total, len - total, 0);
        if (n < 0)
        {
            perror("recv()");
            exit(errno);
        }
        if (n == 0)
        {
            return total;
        }
        total += n;
    }

    return total;
}

void write_server(SOCKET sock, const char *buffer)
{
    if (send(sock, buffer, strlen(buffer), 0) < 0)
    {
        perror("send()");
        exit(errno);
    }
}

int main(int argc, char **argv)
{
    if (argc < 3)
    {
        printf("Usage : %s [address] [pseudo]\n", argv[0]);
        return EXIT_FAILURE;
    }

    init();

    app(argv[1], argv[2]);

    end();

    return EXIT_SUCCESS;
}
