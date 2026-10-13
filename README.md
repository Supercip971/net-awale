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
