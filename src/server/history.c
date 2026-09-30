#include "history.h"
#include <stdio.h>
#include "shared/json/cJSON.h"

FILE *game_hist = NULL;
cJSON *game_state = NULL;

void gameHistoryInit(const char *path)
{
    game_hist = fopen(path, "rw+");
    if (game_hist == NULL)
    {
        perror("Failed to open game history file");
        exit(EXIT_FAILURE);
    }

    fseek(game_hist, 0, SEEK_END);
    long size = ftell(game_hist);
    if (size > 0)
    {
        fseek(game_hist, 0, SEEK_SET);
    }

    if (size == 0)
    {
        // init
        game_state = cJSON_Parse("[]");
    }
    else
    {
        // load
        char *json_str = (char *)malloc(size + 1);
        fread(json_str, 1, size, game_hist);
        json_str[size] = '\0';

        game_state = cJSON_Parse(json_str);

        free(json_str);
    }

    if (game_state == NULL)
    {
        perror("Failed to parse game state");
        exit(EXIT_FAILURE);
    }
}

void gameHistoryDeinit()
{
    fclose(game_hist);
}

static cJSON *gameHistoryEncode(GameHistory *gh)
{
    cJSON *root = cJSON_Parse("{}");
    if (root == NULL)
    {
        perror("Failed to parse game state");
        exit(EXIT_FAILURE);
    }

    auto p1 = cJSON_CreateString(gh->p1);
    auto p2 = cJSON_CreateString(gh->p2);

    cJSON_AddItemToObject(root, "p1", p1);
    cJSON_AddItemToObject(root, "p2", p2);

    auto winner = cJSON_CreateNumber(gh->winner);

    cJSON_AddItemToObject(root, "winner", winner);

    auto plays = cJSON_CreateArray();

    for (int i = 0; i < gh->turns.length; i++)
    {
        auto play = cJSON_CreateString(gh->turns.data[i].play);
        cJSON_AddItemToArray(plays, play);
    }

    cJSON_AddItemToObject(root, "plays", plays);
    return root;
}

static GameHistory *gameHistoryDecode(cJSON *gObject)
{
    GameHistory *gh = malloc(sizeof(GameHistory));

    gh->p1 = cJSON_GetObjectItem(gObject, "p1")->valuestring;
    gh->p2 = cJSON_GetObjectItem(gObject, "p2")->valuestring;
    gh->winner = cJSON_GetObjectItem(gObject, "winner")->valuedouble;

    vec_init(&gh->turns);

    auto plays = cJSON_GetObjectItem(gObject, "plays");
    for (int i = 0; i < cJSON_GetArraySize(plays); i++)
    {
        auto play = cJSON_GetArrayItem(plays, i);

        char *play_str = cJSON_Print(play);
        vec_push(&gh->turns, (Turn){.play = play_str});
    }
    return gh;
}

void gameHistoryPersist(GameHistory *hist)
{
    cJSON *gObject = gameHistoryEncode(hist);
    cJSON_AddItemToArray(game_state, gObject);

    // we write each time
    char *json_str = cJSON_Print(game_state);
    fwrite(json_str, 1, strlen(json_str), game_hist);
    free(json_str);
    return;
}

void gameHistoryForEach(void (*callback)(GameHistory *elt, void *), void* ctx)
{
    for (int i = 0; i < cJSON_GetArraySize(game_state); i++)
    {
        cJSON *gObject = cJSON_GetArrayItem(game_state, i);
        GameHistory *gh = gameHistoryDecode(gObject);
        callback(gh, ctx);

        for (int j = 0; j < gh->turns.length; j++)
        {
            free(gh->turns.data[j].play);
        }
        vec_deinit(&gh->turns);
        free(gh);
    }
}
