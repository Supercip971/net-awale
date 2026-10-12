#ifndef CLIENT_H
#define CLIENT_H

#include "shared/models/serverClient.h"
#ifdef WIN32

#    include <winsock2.h>

#elif defined(__linux__) || defined(__APPLE__)

#    include <arpa/inet.h>
#    include <netdb.h> /* gethostbyname */
#    include <netinet/in.h>
#    include <sys/socket.h>
#    include <sys/types.h>
#    include <unistd.h> /* close */
#    define INVALID_SOCKET -1
#    define SOCKET_ERROR -1
#    define closesocket(s) close(s)
typedef int SOCKET;
typedef struct sockaddr_in SOCKADDR_IN;
typedef struct sockaddr SOCKADDR;
typedef struct in_addr IN_ADDR;

#else

#    error not defined for this platform

#endif

#define CRLF "\r\n"
#define PORT 1977

#define BUF_SIZE 1024

void init(void);
void end(void);
void app(const char *address, const char *name);
int init_connection(const char *address);
void end_connection(int sock);
int read_server(SOCKET sock, char *buffer, size_t len);
void write_server(SOCKET sock, const char *buffer);

void receive_message(SOCKET sock, Server2ClientMessage *msg);
#endif /* guard */
