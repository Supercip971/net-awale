#pragma once

#include <stdint.h>
#include "shared/common.h"
#include "shared/models/const.h"
#include "shared/vec.h"

#define INVALID_PLAYER_ID ((PlayerId)(-1))

typedef vec_t(PlayerId) Friends;

typedef struct Player
{
    PlayerId user_id;
    char name[MAX_USERNAME_LENGTH];
    char bio[MAX_BIO_LENGTH];
    int rank;
    Friends friends;
} Player;

typedef vec_t(Player) Players;
void players_db_init();
void players_db_deinit();

void update_player(Player *player);
PlayerId create_player(Player *player);

void players_leaderboard(Players *output);
Player *find_player_by_id(PlayerId id);
Player *find_player_by_name(char const *name);

int has_friend(Player *p, PlayerId id);
