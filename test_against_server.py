
import os
import socket
import subprocess
import sys
import time

HOST = "127.0.0.1"
PORT = 1977
REPO_DIR = os.path.dirname(os.path.abspath(__file__))
SERVER_BIN = os.path.join(REPO_DIR, "build", "server")
DEFAULT_GAMES_FILE = os.path.join(REPO_DIR, "games.txt")
games_file = DEFAULT_GAMES_FILE

def recv_all(sock: socket.socket) -> str:
    sock.settimeout(0.03)
    data = []
    try:
        while True:
            chunk = sock.recv(4096)
            if not chunk:
                break
            data.append(chunk)
    except (socket.timeout, BlockingIOError):
        pass
    return b"".join(data).decode("utf-8", errors="ignore")


def connect_players(p1: str, p2: str):
    # créé 2 joueur
    s1 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s1.connect((HOST, PORT))
    s1.sendall(f"{p1}\n".encode())
    recv_all(s1)

    s2 = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    s2.connect((HOST, PORT))
    s2.sendall(f"{p2}\n".encode())
    recv_all(s2)
    return s1, s2


def start_game(p1: str, p2: str):
    """Challenge and accept until p1 (South) plays first."""
    for _ in range(20):
        s1, s2 = connect_players(p1, p2)
        s1.sendall(f"defy {p2}\n".encode())
        recv_all(s2)

        s2.sendall(f"accept {p1}\n".encode())
        b1 = recv_all(s1)
        recv_all(s2)

        if f"Au tour de {p1}" in b1:
            return s1, s2

        s1.close()
        s2.close()

    raise RuntimeError("Could not start game with South playing first.")


def replay_game(moves: str, game_num: int):
    p1 = f"P1_{game_num}"
    p2 = f"P2_{game_num}"
    s1, s2 = start_game(p1, p2)

    try:
        winner = "None (Ongoing)"
        moves_played = len(moves)

        for step, ch in enumerate(moves):
            # converti les AAAAA en aaaaah
            # converti les A/a en 0, ...
            hole = ord(ch.upper()) - ord("A")
            sock = s1 if ch.isupper() else s2
            sock.sendall(f"play {hole}\n".encode())

            r1 = recv_all(s1)
            r2 = recv_all(s2)
            resp = r1 + r2
            if "respecte les règle" in resp:
                print(
                    f"[Game {game_num}] Move {step + 1} ('{ch}') rejected by server!"
                )
                return False

            if "T'as gagné" in r1:
                winner = "South (P1)"
                moves_played = step + 1
                break
            elif "T'as gagné" in r2:
                winner = "North (P2)"
                moves_played = step + 1
                break
            elif "Egalité" in resp:
                winner = "Draw"
                moves_played = step + 1
                break

        print(
            f"[Game {game_num}] Accepted ({moves_played}/{len(moves)} moves)  Winner: {winner}"
        )
        return True
    finally:
        s1.close()
        s2.close()


if not os.path.exists(games_file):
    print(f"File '{games_file}' not found.")
    sys.exit(1)

with open(games_file) as f:
    lines = [line.strip() for line in f]

print(
    f"Testing {len(lines)} games from '{os.path.basename(games_file)}' against server...\n"
)
all_passed = all(replay_game(line, i) for i, line in enumerate(lines, 1))
print(
    "\n" + ("success: All games passed!" if all_passed else "error: Some games failed!")
)
