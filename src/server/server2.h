#pragma once

#include "server/client2.h"
#include "server/common.h"

void init(void);
void end(void);
void app(void);
int init_connection(void);
void end_connection(int sock);
int read_client(SOCKET sock, char *buffer);
void send_message_to_all_clients(Clients *clients, Client client, const char *buffer, char from_server);
void remove_client(Clients *clients, int to_remove);
void clear_clients(Clients *clients);
