#pragma once

#include "server/client2.h"
#include "server/common.h"
#include "server/history.h"
#include "shared/game.h"
#include "shared/models/clientServer.h"
#include "shared/vec.h"

void write_client_status(SOCKET sock, const char *buffer);

void write_client_error(SOCKET sock, const char *error);

void write_client_stopstream(SOCKET sock);

// a list of player
void write_client_players(SOCKET sock, vec_str_t players);

void write_client_info(SOCKET sock, const char *name, const char *bio, int rank);

void write_games(SOCKET sock, Games *games);

void write_gameboard(SOCKET sock, Game *game);

void write_defy_notify(SOCKET sock, const char *challenger);

void write_spectator_notify(SOCKET sock, const char *spectator);

void write_winner_notify(SOCKET sock, const char *winner, bool is_draw);

void write_friends(SOCKET sock, vec_str_t friends);

void write_game_histories(SOCKET sock, GameHistories *histories);
