#include "player.h"
#include <stdio.h>
#include "shared/json/cJSON.h"
#include "shared/models/const.h"

// for now a big fat table but we would love to make it dynamic and persistent in the future

Players memory_db = {};

PlayerId last_used_id = 0;
FILE *player_persist = NULL;

// PLAYER.json : {
//      "user_id": 1012823,
//      "name": "bernard",
//      "bio": "",
//      "rank": 1,
// }
void decode_json(cJSON *players)
{
    cJSON *player;
    cJSON_ArrayForEach(player, players)
    {
        Player p = {};
        p.user_id = cJSON_GetObjectItem(player, "user_id")->valueint;
        strncpy(p.name, cJSON_GetObjectItem(player, "name")->valuestring, MAX_USERNAME_LENGTH);
        strncpy(p.bio, cJSON_GetObjectItem(player, "bio")->valuestring, MAX_BIO_LENGTH);
        p.rank = cJSON_GetObjectItem(player, "rank")->valueint;
        vec_init(&p.friends);

        if (p.user_id > last_used_id)
            last_used_id = p.user_id;

        cJSON *friends = NULL;

        // retrocompatibility against old db without any friends field
        if (!cJSON_HasObjectItem(player, "friends"))
        {
            vec_push(&memory_db, p);
            continue;
        }

        friends = cJSON_GetObjectItem(player, "friends");

        cJSON *friend;
        // add each friend
        cJSON_ArrayForEach(friend, friends)
        {
            vec_push(&p.friends, cJSON_GetNumberValue(friend));
        }

        vec_push(&memory_db, p);
    }

    printf("loaded %d players\n", cJSON_GetArraySize(players));
}

cJSON *encode_json()
{
    cJSON *players = cJSON_Parse("[]");

    for (int i = 0; i < memory_db.length; i++)
    {
        cJSON *player = cJSON_CreateObject();
        Player *p = &memory_db.data[i];
        cJSON_AddNumberToObject(player, "user_id", p->user_id);
        cJSON_AddStringToObject(player, "name", p->name);
        cJSON_AddStringToObject(player, "bio", p->bio);
        cJSON_AddNumberToObject(player, "rank", p->rank);

        cJSON *friends = cJSON_CreateArray();
        for (int j = 0; j < p->friends.length; j++)
        {
            cJSON *friend_id = cJSON_CreateNumber(p->friends.data[j]);
            cJSON_AddItemToArray(friends, friend_id);
        }

        cJSON_AddItemToObject(player, "friends", friends);
        cJSON_AddItemToArray(players, player);
    }

    return players;
}

void players_db_init()
{
    vec_init(&memory_db);
    player_persist = fopen("players.json", "r+");

    if (player_persist == NULL)
    {
        perror("Failed to open players.json");
        exit(EXIT_FAILURE);
    }

    fseek(player_persist, 0, SEEK_END);

    long size = ftell(player_persist);
    char *memory = malloc(size + 1);
    cJSON *players;
    if (size > 0)
    {
        fseek(player_persist, 0, SEEK_SET);
        fread(memory, size, 1, player_persist);
        memory[size] = '\0';
        players = cJSON_Parse(memory);
        free(memory);
    }
    else
    {
        players = cJSON_Parse("[]");
    }

    if (players == NULL)
    {
        perror("Failed to parse players.json");
        exit(EXIT_FAILURE);
    }

    decode_json(players);
}

void players_db_deinit()
{
    cJSON *players = encode_json();
    char *json = cJSON_Print(players);
    fseek(player_persist, 0, SEEK_SET);
    fwrite(json, strlen(json), 1, player_persist);
    free(json);
    cJSON_Delete(players);
    fclose(player_persist);
    for (int i = 0; i < memory_db.length; ++i)
    {
        vec_deinit(&memory_db.data[i].friends);
    }
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
    last_used_id++;
    player->user_id = last_used_id;
    vec_push(&memory_db, *player);
    return last_used_id;
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

int has_friend(Player *p, PlayerId id)
{
    if (p == NULL)
        return 0;
    for (int k = 0; k < p->friends.length; ++k)
    {
        if (p->friends.data[k] == id)
            return 1;
    }
    return 0;
}
