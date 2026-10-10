#include "player.h"
#include "shared/models/const.h"


// for now a big fat table but we would love to make it dynamic and persistent in the future

Players memory_db = {};

void players_db_init()
{
    vec_init(&memory_db);
}
void players_db_deinit()
{
    vec_deinit(&memory_db);
}

void update_player(Player *player)
{
    for (int i = 0; i < memory_db.length; ++i)
    {
        if (memory_db.data[i].user_id == player->user_id)
        {
            memory_db.data[i] = *player;
            return;
        }
    }
}

PlayerId create_player(Player *player)
{
    PlayerId id = memory_db.length;
    player->user_id = id;
    vec_push(&memory_db, *player);
    return id;
}

Player *find_player_by_id(PlayerId id)
{
    for (int i = 0; i < memory_db.length; ++i)
    {
        if (memory_db.data[i].user_id == id)
        {
            return &memory_db.data[i];
        }
    }
    return NULL;
}

Player *find_player_by_name(char const *name)
{
    for (int i = 0; i < memory_db.length; ++i)
    {
        if (strcmp(memory_db.data[i].name, name) == 0)
        {
            return &memory_db.data[i];
        }
    }
    return NULL;
}
