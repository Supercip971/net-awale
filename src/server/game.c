#include "game.h"
#include "client2.h"
#include "server/message.h"
#include "server/player.h"

void gameInit(Game *game)
{
    // init at 0
    *game = (Game){0};

    // 4 seed per hole
    for (int i = 0; i < 12; i++)
        game->board[i] = 4;

    game->status = WAITING;
    int randomIndex = rand() % 2;
    game->currentPlayer = randomIndex;
    game->private = 0;
    vec_init(&game->turns);
}

static int sideSum(const Game *g, int side)
{
    int s = 0;
    for (int i = 0; i < 6; i++)
        s += g->board[i + side * 6];
    return s;
}

int play(Game *game, int player, int hole, int *capturedSeeds)
{
    *capturedSeeds = 0;
    // check if the hole is valid
    if (hole < 0 || hole > 5)
        return 0;

    int start = hole + player * 6;
    int opp = 1 - player;

    // check if the player has seeds in the hole
    if (game->board[start] == 0)
        return 0;

    Game simulation = *game;
    int seeds = simulation.board[start];
    simulation.board[start] = 0;
    int cur = start;
    while (seeds > 0)
    {
        cur = (cur + 1) % 12;
        if (cur == start)
            continue; // skip origin hole
        simulation.board[cur]++;
        --seeds;
    }

    // feeding rule: if the opponent was starving, the move must feed him
    if (sideSum(game, opp) == 0 && sideSum(&simulation, opp) == 0)
        return 0;

    // captures: only after the last seed, going backwards on the opponent's side
    Game cap = simulation;
    int gained = 0;
    int c = cur;
    while (c / 6 == opp && (cap.board[c] == 2 || cap.board[c] == 3))
    {
        gained += cap.board[c];
        cap.board[c] = 0;
        c = (c + 11) % 12;
    }

    // grand slam: capturing everything is allowed but captures nothing
    if (gained > 0 && sideSum(&cap, opp) == 0)
    {
        gained = 0;
        cap = simulation;
    }

    *game = cap;
    game->hands[player] += gained;
    *capturedSeeds = gained;
    return 1;
}

int hasLegalMove(const Game *game, int player)
{
    for (int h = 0; h < 6; h++)
    {
        Game copy = *game;
        int captured;
        if (play(&copy, player, h, &captured))
            return 1;
    }
    return 0;
}

GameStatus gameStatus(Game *game, int nextPlayer)
{
    // 25+ seeds captured: immediate win
    if (game->hands[0] >= 25)
        return GAME_WIN_P0;
    if (game->hands[1] >= 25)
        return GAME_WIN_P1;

    // nobody can move: each player keeps the seeds on his own side
    if (!hasLegalMove(game, nextPlayer))
    {
        for (int p = 0; p < 2; p++)
        {
            game->hands[p] += sideSum(game, p);
            for (int i = 0; i < 6; i++)
                game->board[i + p * 6] = 0;
        }
        if (game->hands[0] > game->hands[1])
            return GAME_WIN_P0;
        if (game->hands[1] > game->hands[0])
            return GAME_WIN_P1;
        return GAME_DRAW;
    }

    return GAME_ONGOING;
}

char *printGame(Game *game, char board[BUF_SIZE])
{
    char *p = board;
    char player1Name[MAX_USERNAME_LENGTH] = {0};
    char player2Name[MAX_USERNAME_LENGTH] = {0};
    Player *p1 = find_player_by_id(game->players[0]);
    Player *p2 = find_player_by_id(game->players[1]);
    Player *curr = find_player_by_id(game->players[game->currentPlayer]);
    snprintf(player1Name, sizeof player1Name, "%s", p1 ? p1->name : "Joueur 1");
    snprintf(player2Name, sizeof player2Name, "%s", p2 ? p2->name : "Joueur 2");
    const char *currName = curr ? curr->name : "Inconnu";
    p += sprintf(p, "Grenier du joueur 1 (%s): %d\n\n"
                    "\n"
                    "\t 0\t\t 1\t\t 2\t\t 3\t\t 4\t\t 5\t\n"
                    "\n"
                    "*-----------------------------------------------------------------------------------------------*\n"
                    "|                                                                                               |\n",
                 player1Name, game->hands[0]);
    for (int i = 0; i < 6; i++)
    {
        p += sprintf(p, "|\t%2d\t", game->board[i]);
    }
    p += sprintf(p, "|\n"
                    "|                                                                                               |\n"
                    "*-----------------------------------------------------------------------------------------------*\n"
                    "|                                                                                               |\n");
    for (int i = 11; i >= 6; --i)
    {
        p += sprintf(p, "|\t%2d\t", game->board[i]);
    }
    p += sprintf(p, "|\n"
                    "|                                                                                               |\n"
                    "*-----------------------------------------------------------------------------------------------*\n"
                    "\n"
                    "\t 5\t\t 4\t\t 3\t\t 2\t\t 1\t\t 0\t\n"
                    "\n");
    p += sprintf(p, "\nGrenier du joueur 2 (%s): %d\n\nAu tour de %s de jouer !", player2Name, game->hands[1], currName);
    return board;
}

char *displayGame(Game *game, Writer *writer)
{
    Player *p1 = find_player_by_id(game->players[0]);
    Player *p2 = find_player_by_id(game->players[1]);
    w_write(writer, "%s VS %s", p1 ? p1->name : "Joueur 1", p2 ? p2->name : "Joueur 2");
    return writer->data;
}

Game *find_game(Games *games, PlayerId player)
{
    for (int i = 0; i < games->length; ++i)
    {
        Game *cur = &games->data[i];
        if (cur->status != IN_GAME)
            continue;

        if (cur->players[0] == player || cur->players[1] == player)
        {
            return cur;
        }
    }
    return NULL;
}

int is_player_in_game(Games *games, PlayerId player)
{
    for (int i = 0; i < games->length; ++i)
    {
        if (games->data[i].status != IN_GAME)
            continue;

        if (games->data[i].players[0] == player || games->data[i].players[1] == player)
        {
            return 1;
        }
    }
    return 0;
}

int is_player_spectating_this_game(Game *game, PlayerId player)
{
    for (int i = 0; i < game->spectatorsCount; ++i)
    {
        if (game->spectators[i] == player)
        {
            return 1;
        }
    }
    return 0;
}

Game *is_player_spectating_a_game(Games *games, PlayerId player)
{
    for (int i = 0; i < games->length; ++i)
    {
        if (is_player_spectating_this_game(&(games->data[i]), player))
        {
            return &games->data[i];
        }
    }
    return NULL;
}

void set_game_private(Game *game, int private, Clients *connections)
{
    game->private = private;
    if (!private)
        return;
    for (int i = 0; i < game->spectatorsCount; ++i)
    {
        for (int j = 0; j < connections->length; ++j)
        {
            if (connections->data[j].player == game->spectators[i])
            {
                write_client(connections->data[j].sock, "T'ES KICK ILS PREFERENT RESTER EN PV\n");
                break;
            }
        }
    }
    game->spectatorsCount = 0;
}
