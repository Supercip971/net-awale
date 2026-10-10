#pragma once

#include <stdint.h>
#include "shared/models/const.h"
#include "shared/vec.h"

typedef uint64_t PlayerId;
#define INVALID_PLAYER_ID ((PlayerId)(-1))

typedef struct Player
{
    PlayerId user_id;
    char name[MAX_USERNAME_LENGTH];
    char bio[MAX_BIO_LENGTH];
    int rank;
} Player;

void players_db_init();
void players_db_deinit();

void update_player(Player *player);
PlayerId create_player(Player *player);

Player *find_player_by_id(PlayerId id);
Player *find_player_by_name(char const *name);

typedef vec_t(Player) Players;
