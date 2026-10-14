# Network Awale 

An app made for INSA Lyon school project.

## Prerequisites

Check that you have `players.json`and `history.json` files at the root of the project (not in src/)

## How to build 

> dependencies:
> - tested on MacOS/Linux not on Windows
> - gcc/clang 
> - (optional:) python for unit test
> - (optional:) tmux for multi-terminal test support

```sh
make all -j
```

## How to run 

### Server 

```sh
./build/server
```

### Client 

```sh
./build/client {IP_HOST} {USERNAME}
```

## Tests

Taken from:  https://github.com/joansalasoler/oware/blob/develop/src/test/resources/oware-bench.suite

## Features

### Commands

These commands explain the most features that we have

| Command | Description |
| --- | --- |
| Help | affiche l'aide des commandes |
| players | affiche la liste des joueurs connectes |
| help | affiche l'aide des commandes |
| players | affiche la liste des joueurs connectes |
| info <player> | affiche les informations / le profil d'un joueur |
| message <text> | envoie un message dans le chat global |
| games | liste les parties en cours |
| defy <player> | defie un joueur en duel |
| accept <player> | accepte le defi d'un joueur |
| decline <player> | refuse le defi d'un joueur |
| cancel <player> | annuler le defi d'un joueur |
| play <hole> | joue une case ou un trou (0 a 5) |
| giveup | abandonne la partie en cours |
| spec <player> | observe la partie d'un joueur |
| stopspec | stop d'observer une partie |
| friends | affiche la liste de vos amis |
| addfriend <player> | ajoute un joueur en ami |
| setbio <bio> | modifie votre biographie |
| setprivate <0/1> | definit la partie privee (1) ou publique (0) |
| history | affiche l'historique des parties terminees |
| replaygame <id> | lance la rediffusion d'une partie |
| s | avance d'un coup dans le replay (step) |
| c | continue le replay automatiquement jusqu'a la fin |

### Other features

We also implemented the elo system (in `src/server/elo.c`)

When a client disconnect, he can join his game when reconnecting. However, it's considered as soft give up. If the opponent gives up when you disconnect, he wins (not to block him).

It's not possible to remove friends, and it's done on purpose. What if your friend is watching and then you remove him? Both choices to kick him or to make him stay wasn't acceptable for us so we chose not to implement it.