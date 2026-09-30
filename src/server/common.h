#pragma once

#ifdef WIN32

#    include <winsock2.h>

#elif defined(__linux__) || defined(__APPLE__)

#    include <arpa/inet.h>
#    include <netdb.h> /* gethostbyname */
#    include <netinet/in.h>
#    include <sys/select.h>
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
#define MAX_CLIENTS 100

#define BUF_SIZE 1024