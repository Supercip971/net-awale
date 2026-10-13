#include "help.h"
#include <stdio.h>

void showHelp()
{
    printf(" = General & Chat:\n");
    printf("  help                     - affiche l'aide des commandes\n");
    printf("  players                  - affiche la liste des joueurs connectes\n");
    printf("  info <player>            - affiche les informations / le profil d'un joueur\n");
    printf("  message <text>           - envoie un message dans le chat global\n");

    printf("\n = Matchmaking & Games:\n");
    printf("  games                    - liste les parties en cours\n");
    printf("  defy <player>            - defie un joueur en duel\n");
    printf("  accept <player>          - accepte le defi d'un joueur\n");
    printf("  decline <player>         - refuse le defi d'un joueur\n");

    printf("\n = During a game:\n");
    printf("  play <hole>              - joue une case ou un trou (0 a 5)\n");
    printf("  giveup                   - abandonne la partie en cours\n");

    printf("\n = Spectating:\n");
    printf("  spec <player>            - observe la partie d'un joueur\n");
    printf("  stopspec                 - stop d'observer une partie\n");

    printf("\n = Friends & Settings:\n");
    printf("  friends                  - affiche la liste de vos amis\n");
    printf("  addfriend <player>       - ajoute un joueur en ami\n");
    printf("  setbio <bio>             - modifie votre biographie\n");
    printf("  setprivate <0/1>         - definit la partie privee (1) ou publique (0)\n");

    printf("\n = History & Replay:\n");
    printf("  history                  - affiche l'historique des parties terminees\n");
    printf("  replaygame <id>          - lance la rediffusion d'une partie\n");
    // inspiré de gdp
    printf("  s                        - avance d'un coup dans le replay (step)\n");
    printf("  c                        - continue le replay automatiquement jusqu'a la fin\n");

    printf("\n$ "); // hack: pour que le prompt soit affiché après l'aide
    fflush(stdout);
}
