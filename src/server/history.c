#include "history.h"
#include <stdio.h>
#include "server/game.h"
#include "shared/json/cJSON.h"

FILE *game_hist = NULL;
GameHistories histories = {};

static cJSON *gameHistoryEncode(GameHistory *gh)
{
    cJSON *root = cJSON_Parse("{}");
    if (root == NULL)
    {
        perror("Failed to parse game state");
        exit(EXIT_FAILURE);
    }

    auto p1 = cJSON_CreateNumber(gh->p1);
    auto p2 = cJSON_CreateNumber(gh->p2);

    cJSON_AddItemToObject(root, "p1", p1);
    cJSON_AddItemToObject(root, "p2", p2);

    auto winner = cJSON_CreateNumber(gh->winner);

    cJSON_AddItemToObject(root, "winner", winner);

    auto plays = cJSON_CreateArray();

    for (int i = 0; i < gh->turns.length; i++)
    {
        auto play = cJSON_CreateNumber(gh->turns.data[i]);
        cJSON_AddItemToArray(plays, play);
    }

    cJSON_AddItemToObject(root, "plays", plays);
    return root;
}

static GameHistory *gameHistoryDecode(cJSON *gObject)
{
    GameHistory *gh = malloc(sizeof(GameHistory));

    gh->p1 = cJSON_GetObjectItem(gObject, "p1")->valueint;
    gh->p2 = cJSON_GetObjectItem(gObject, "p2")->valueint;
    gh->winner = cJSON_GetObjectItem(gObject, "winner")->valuedouble;

    vec_init(&gh->turns);

    auto plays = cJSON_GetObjectItem(gObject, "plays");
    for (int i = 0; i < cJSON_GetArraySize(plays); i++)
    {
        auto play = cJSON_GetArrayItem(plays, i);
        vec_push(&gh->turns, cJSON_GetNumberValue(play));
    }
    return gh;
}

void gameHistoryInit()
{
    cJSON *game_state = NULL;
    game_hist = fopen("history.json", "rw+");
    if (game_hist == NULL)
    {
        fprintf(stderr, "Failed to open game history file: create empty history.json\n");
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

        for (int i = 0; i < cJSON_GetArraySize(game_state); i++)
        {
            cJSON *gObject = cJSON_GetArrayItem(game_state, i);
            GameHistory *gh = gameHistoryDecode(gObject);
            vec_push(&histories, *gh);
        }

        cJSON_Delete(game_state);
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
    cJSON *game_state = cJSON_Parse("[]");
    if (game_state == NULL)
    {
        perror("Failed to parse game state");
        exit(EXIT_FAILURE);
    }

    for (int i = 0; i < histories.length; i++)
    {
        GameHistory *current = &histories.data[i];
        cJSON *gObject = gameHistoryEncode(current);
        cJSON_AddItemToArray(game_state, gObject);
        vec_deinit(&current->turns);
    }

    char *json_str = cJSON_Print(game_state);
    fseek(game_hist, 0, SEEK_SET);
    fwrite(json_str, 1, strlen(json_str), game_hist);
    free(json_str);
    cJSON_Delete(game_state);
    fclose(game_hist);
}

static void gameHistoryPersist(GameHistory *hist)
{
    GameHistory h = *hist;
    vec_push(&histories, h);
    return;
}

void gameHistoryGameEnded(Game *game, int winner)
{
    GameHistory hist = {
        .turns = game->turns,
        .p1 = game->players[0],
        .p2 = game->players[1],
        .winner = game->players[winner],
    };
    gameHistoryPersist(&hist);
}

void gameHistoryForEach(void (*callback)(GameHistory *elt, void *), void *ctx, int count)
{
    for (int i = histories.length - 1; i >= 0 && i >= histories.length - count; i--)
    {
        GameHistory *gh = &histories.data[i];
        callback(gh, ctx);
    }
}

void gameHistoryForEachPlayer(PlayerId of, void (*callback)(GameHistory *elt, void *ctx), void *ctx, int count)
{
    for (int i = histories.length - 1; i >= 0 && i >= histories.length - count; i--)
    {
        GameHistory *gh = &histories.data[i];
        if(of == gh->p1 || of == gh->p2)
        {
            callback(gh, ctx);
        }
    }
}
