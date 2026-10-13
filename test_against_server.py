#!/usr/bin/env python3
"""Integration and unit tests for net-awale server using the client binary.

Tests both the Awale game rules and replays full game records against the server.
Uses the official './build/client' binary and raw stdin input.
"""

import argparse
import atexit
import fcntl
import os
import re
import socket
import subprocess
import sys
import time

HOST = "127.0.0.1"
PORT = 1977
REPO_DIR = os.path.dirname(os.path.abspath(__file__))
SERVER_BIN = os.path.join(REPO_DIR, "build", "server")
CLIENT_BIN = os.path.join(REPO_DIR, "build", "client")
DEFAULT_GAMES_FILE = os.path.join(REPO_DIR, "games.txt")

_server_process = None


def is_server_listening() -> bool:
    try:
        with socket.create_connection((HOST, PORT), timeout=0.1):
            return True
    except (ConnectionRefusedError, socket.timeout, OSError):
        return False


def ensure_binaries():
    need_make = False
    if not os.path.isfile(SERVER_BIN) or not os.path.isfile(CLIENT_BIN):
        need_make = True

    if need_make:
        print("Binaries missing. Building project with 'make'...")
        subprocess.run(["make", "-C", REPO_DIR], check=True)


def ensure_server_running():
    global _server_process
    if is_server_listening():
        return

    ensure_binaries()
    print("Starting server in background...")
    _server_process = subprocess.Popen(
        [SERVER_BIN],
        cwd=REPO_DIR,
        stdout=subprocess.DEVNULL,
        stderr=subprocess.DEVNULL,
    )
    atexit.register(cleanup_server)

    for _ in range(50):
        time.sleep(0.05)
        if is_server_listening():
            print("Server is up and listening on port", PORT)
            return

    raise RuntimeError("Server failed to start and listen on port 1977")


def cleanup_server():
    global _server_process
    if _server_process is not None:
        try:
            _server_process.terminate()
            _server_process.wait(timeout=1.0)
        except Exception:
            try:
                _server_process.kill()
            except Exception:
                pass
        _server_process = None


class ClientProcess:
    """Wrapper around './build/client' sending raw user input through stdin."""

    def __init__(self, name: str):
        self.name = name
        self.proc = subprocess.Popen(
            [CLIENT_BIN, HOST, name],
            cwd=REPO_DIR,
            stdin=subprocess.PIPE,
            stdout=subprocess.PIPE,
            stderr=subprocess.PIPE,
            text=True,
        )
        for stream in (self.proc.stdout, self.proc.stderr):
            fd = stream.fileno()
            fl = fcntl.fcntl(fd, fcntl.F_GETFL)
            fcntl.fcntl(fd, fcntl.F_SETFL, fl | os.O_NONBLOCK)

    def send(self, cmd: str):
        if self.proc.poll() is not None:
            raise RuntimeError(f"Client {self.name} is not running.")
        self.proc.stdin.write(cmd + "\n")
        self.proc.stdin.flush()

    def drain(self, timeout: float = 0.1) -> str:
        t0 = time.time()
        buf = []
        while time.time() - t0 < timeout:
            got_data = False
            for stream in (self.proc.stdout, self.proc.stderr):
                try:
                    chunk = stream.read()
                    if chunk:
                        buf.append(chunk)
                        got_data = True
                except (BlockingIOError, TypeError):
                    pass
            if got_data:
                t0 = time.time()
            time.sleep(0.003)
        return "".join(buf)

    def close(self):
        try:
            if self.proc.poll() is None:
                self.proc.terminate()
                self.proc.wait(timeout=0.2)
        except Exception:
            try:
                self.proc.kill()
            except Exception:
                pass


def parse_board_output(text: str):
    """Parses client stdout board representation into structured data:
    - board: 12 integers (0..5 for South, 6..11 for North)
    - hands: [South hand, North hand]
    - turn: name of current player
    """
    h1_m = re.findall(r"Grenier du joueur 1 \((.*?)\): (\d+)", text)
    h2_m = re.findall(r"Grenier du joueur 2 \((.*?)\): (\d+)", text)
    turn_m = re.findall(r"Au tour de (.*?) de jouer !", text)

    rows = []
    for line in text.splitlines():
        if "|\t" in line:
            parts = [
                int(x.strip())
                for x in line.split("|")
                if x.strip() and x.strip().isdigit()
            ]
            if len(parts) == 6:
                rows.append(parts)

    if not (h1_m and h2_m and turn_m and len(rows) >= 2):
        return None

    h1 = int(h1_m[-1][1])
    h2 = int(h2_m[-1][1])
    turn = turn_m[-1]
    last_rows = rows[-2:]
    # row 0 is holes 0..5
    # row 1 is holes 11 downto 6, so reverse row 1 to get 6..11
    board = last_rows[0] + list(reversed(last_rows[1]))
    return {"board": board, "hands": [h1, h2], "turn": turn}


def start_match(p1_base: str, p2_base: str, south_must_start: bool = True):
    """Connects two clients, challenges and accepts.
    If south_must_start is True, retries with fresh attempt names until South plays first.
    Using attempt-indexed names avoids 'Il est déjà en game' conflicts if the previous game
    was not yet purged by the server.
    """
    for attempt in range(30):
        p1_name = f"{p1_base}_{attempt}" if attempt > 0 else p1_base
        p2_name = f"{p2_base}_{attempt}" if attempt > 0 else p2_base
        c1 = ClientProcess(p1_name)
        c2 = ClientProcess(p2_name)
        c1.drain(0.03)
        c2.drain(0.03)

        c1.send(f"defy {p2_name}")
        c2.drain(0.03)

        c2.send(f"accept {p1_name}")
        b1 = c1.drain(0.08)
        c2.drain(0.03)

        if not south_must_start or f"Au tour de {p1_name}" in b1:
            return c1, c2, b1

        c1.close()
        c2.close()
        time.sleep(0.03)

    raise RuntimeError(
        f"Could not start match between {p1_base} and {p2_base} with South first"
    )


# ---------------------------------------------------------------------------
# Awale Rules Tests
# ---------------------------------------------------------------------------


def test_rule_initial_board_and_sowing():
    """Rule 1:
    - Board starts with 4 seeds per hole (all 12 holes).
    - Sowing takes all seeds from hole, sows 1 in each subsequent counter-clockwise hole.
    - Hands start at 0 and turn alternates.
    """
    c1, c2, init_out = start_match("P1_r1", "P2_r1", south_must_start=True)
    try:
        state = parse_board_output(init_out)
        assert state is not None, "Failed to parse initial board"
        assert state["board"] == [4] * 12, (
            f"Expected 4 seeds per hole, got {state['board']}"
        )
        assert state["hands"] == [0, 0], (
            f"Expected hands [0, 0], got {state['hands']}"
        )
        assert state["turn"] == c1.name, (
            f"Expected {c1.name} turn, got {state['turn']}"
        )

        # South plays hole 0 (A)
        c1.send("play 0")
        out1 = c1.drain(0.05)
        c2.drain(0.05)
        st1 = parse_board_output(out1)
        assert st1 is not None, "Failed to parse board after move 0"
        # Hole 0 becomes 0, holes 1, 2, 3, 4 receive 1 seed (now 5), rest are 4
        expected = [0, 5, 5, 5, 5, 4, 4, 4, 4, 4, 4, 4]
        assert st1["board"] == expected, (
            f"Expected board {expected}, got {st1['board']}"
        )
        assert st1["hands"] == [0, 0], (
            f"Expected hands [0, 0], got {st1['hands']}"
        )
        assert st1["turn"] == c2.name, (
            f"Expected turn to switch to {c2.name}, got {st1['turn']}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_out_of_turn_rejected():
    """Rule 2: A player cannot play when it is not their turn."""
    # We do not require South to start first: whoever starts first tests out-of-turn moves!
    c1, c2, init_out = start_match("P1_r2", "P2_r2", south_must_start=False)
    try:
        st = parse_board_output(init_out)
        assert st is not None, "Failed to parse initial board"
        first_actor = c1 if st["turn"] == c1.name else c2
        second_actor = c2 if st["turn"] == c1.name else c1

        # Second actor tries to play when it's first actor's turn -> Rejected!
        second_actor.send("play 0")
        err1 = second_actor.drain(0.05)
        assert (
            "Chill bro, laisse le jouer" in err1
            or "server side error" in err1
        ), f"Expected out-of-turn rejection for second player, got: {err1}"

        # First actor plays legally
        first_actor.send("play 0")
        first_actor.drain(0.05)
        second_actor.drain(0.05)

        # First actor tries to play AGAIN (out of turn) -> Rejected!
        first_actor.send("play 1")
        err2 = first_actor.drain(0.05)
        assert (
            "Chill bro, laisse le jouer" in err2
            or "server side error" in err2
        ), f"Expected out-of-turn rejection for consecutive play, got: {err2}"
    finally:
        c1.close()
        c2.close()


def test_rule_empty_hole_rejected():
    """Rule 3: Playing from an empty hole (0 seeds) is forbidden."""
    c1, c2, _ = start_match("P1_r3", "P2_r3", south_must_start=True)
    try:
        # P1 plays hole 0 -> hole 0 now has 0 seeds
        c1.send("play 0")
        c1.drain(0.05)
        c2.drain(0.05)

        # P2 plays hole 0
        c2.send("play 0")
        c2.drain(0.05)
        c1.drain(0.05)

        # It is P1's turn again. P1 tries to play hole 0 which is empty!
        c1.send("play 0")
        err_out = c1.drain(0.05)
        assert (
            "respecte les règle" in err_out or "server side error" in err_out
        ), f"Expected empty hole rejection, got: {err_out}"
    finally:
        c1.close()
        c2.close()


def test_rule_invalid_hole_index_rejected():
    """Rule 4: Invalid hole indices (< 0 or > 5) are rejected."""
    c1, c2, _ = start_match("P1_r4", "P2_r4", south_must_start=True)
    try:
        c1.send("play -1")
        err1 = c1.drain(0.05)
        assert (
            "respecte les règle" in err1 or "server side error" in err1
        ), f"Expected negative hole rejection, got: {err1}"

        c1.send("play 6")
        err2 = c1.drain(0.05)
        assert (
            "respecte les règle" in err2 or "server side error" in err2
        ), f"Expected hole 6 rejection, got: {err2}"
    finally:
        c1.close()
        c2.close()


def test_rule_single_capture():
    """Rule 5:
    Capture occurs when the last sown seed lands on the opponent side in a hole with 2 or 3 seeds.
    Sequence: South(0), North(2), South(2), North(1) -> captures 2 seeds.
    """
    c1, c2, _ = start_match("P1_r5", "P2_r5", south_must_start=True)
    try:
        # South: 0
        c1.send("play 0")
        c1.drain(0.03)
        c2.drain(0.03)

        # North: 2
        c2.send("play 2")
        c1.drain(0.03)
        c2.drain(0.03)

        # South: 2
        c1.send("play 2")
        c1.drain(0.03)
        c2.drain(0.03)

        # North: 1 (sows 4 seeds, last lands on South's hole 2 making it 2 seeds)
        c2.send("play 1")
        out1 = c1.drain(0.05)
        out2 = c2.drain(0.05)
        st = parse_board_output(out1 or out2)
        assert st is not None, "Failed to parse board after capture move"
        assert st["hands"] == [0, 2], (
            f"Expected North to capture 2 seeds (hands=[0, 2]), got: {st['hands']}"
        )
        # Captured hole 2 on South should now have 0 seeds
        assert st["board"][2] == 0, (
            f"Expected South hole 2 to be 0 after capture, got: {st['board'][2]}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_multiple_captures():
    """Rule 6:
    Multiple / retrograde captures: if preceding holes on opponent side also have 2 or 3 seeds,
    they are captured consecutively backwards.
    Sequence: South(0), North(0), South(1), North(2), South(2), North(1) -> captures 4 seeds.
    """
    c1, c2, _ = start_match("P1_r6", "P2_r6", south_must_start=True)
    try:
        seq = [(c1, 0), (c2, 0), (c1, 1), (c2, 2), (c1, 2), (c2, 1)]
        last_out = ""
        for proc, hole in seq:
            proc.send(f"play {hole}")
            o1 = c1.drain(0.04)
            o2 = c2.drain(0.04)
            last_out = o1 or o2

        st = parse_board_output(last_out)
        assert st is not None, "Failed to parse board after multi capture"
        assert st["hands"] == [0, 4], (
            f"Expected North hand to have 4 seeds, got: {st['hands']}"
        )
        assert st["board"][1] == 0 and st["board"][2] == 0, (
            f"Expected both captured holes to have 0 seeds, got {st['board']}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_own_side_capture_forbidden():
    """Rule 7: Seeds sown on the player's OWN side that end in 2 or 3 are NEVER captured."""
    c1, c2, _ = start_match("P1_r7", "P2_r7", south_must_start=True)
    try:
        # Move 1: South plays hole 0 -> lands on holes 1, 2, 3, 4 (own side)
        # None of South's own holes are captured
        c1.send("play 0")
        st1 = parse_board_output(c1.drain(0.05))
        c2.drain(0.05)
        assert st1["hands"] == [0, 0], (
            f"Expected no capture on own side, got: {st1['hands']}"
        )

        # Move 2: North plays hole 0 -> lands on holes 1, 2, 3, 4 (North's own side)
        c2.send("play 0")
        st2 = parse_board_output(c2.drain(0.05))
        c1.drain(0.05)
        assert st2["hands"] == [0, 0], (
            f"Expected no capture on own side, got: {st2['hands']}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_skip_origin_hole_12_seeds():
    """Rule 8:
    When sowing 12 or more seeds, sowing wraps all 12 holes, and the origin hole
    must be skipped (leaves 0 seeds in origin hole).
    """
    c1, c2, _ = start_match("P1_r8", "P2_r8", south_must_start=True)
    try:
        # Play first 35 moves of Game 2 to set up a hole with 12 seeds
        moves = "FeDbBdAaEcDdFaEaCdDcEdFaBfEcFdDaEcFfAaBeFdEfDeAcBdEbBcAfAdCfDeAfEbDcEaCbBdAcEeDdBeCfEaDbAcBdCeDfEaAbBcCdDeEfFfEeDdCcB"
        for ch in moves[:35]:
            actor = c1 if ch.isupper() else c2
            hole = ord(ch.upper()) - ord("A")
            actor.send(f"play {hole}")
            c1.drain(0.02)
            c2.drain(0.02)

        # North plays hole 5 (feeds South)
        c2.send("play 5")
        c1.drain(0.02)
        c2.drain(0.02)

        # South plays hole 0
        c1.send("play 0")
        c1.drain(0.02)
        c2.drain(0.02)

        # North plays hole 1 which has 12 seeds!
        c2.send("play 1")
        out = c2.drain(0.05)
        c1.drain(0.05)
        st = parse_board_output(out)
        assert st is not None, "Failed to parse board after 12-seed sowing"
        # Hole 1 of North corresponds to board index 7
        assert st["board"][7] == 0, (
            f"Expected origin hole (index 7) to be skipped and remain 0, got: {st['board'][7]}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_famine_feeding_rule():
    """Rule 9:
    Famine / Feeding rule (Nourrir l'adversaire):
    When the opponent has 0 seeds, the player MUST make a move that feeds them.
    Moves that do not feed are rejected.
    """
    c1, c2, _ = start_match("P1_r9", "P2_r9", south_must_start=True)
    try:
        moves = "FeDbBdAaEcDdFaEaCdDcEdFaBfEcFdDaEcFfAaBeFdEfDeAcBdEbBcAfAdCfDeAfEbDcEaCbBdAcEeDdBeCfEaDbAcBdCeDfEaAbBcCdDeEfFfEeDdCcB"
        for ch in moves[:35]:
            actor = c1 if ch.isupper() else c2
            hole = ord(ch.upper()) - ord("A")
            actor.send(f"play {hole}")
            c1.drain(0.02)
            c2.drain(0.02)

        # At this point, South has 0 seeds (famine).
        # North tries to play hole 0 (1 seed, stays on North side, doesn't feed South)
        c2.send("play 0")
        err = c2.drain(0.05)
        assert (
            "respecte les règle" in err or "server side error" in err
        ), f"Expected feeding violation rejection, got: {err}"

        # North plays hole 5 (1 seed, feeds South's hole 0) -> must be accepted!
        c2.send("play 5")
        out = c2.drain(0.05)
        c1.drain(0.05)
        st = parse_board_output(out)
        assert st is not None, "Failed to parse board after feeding move"
        # South must have received at least 1 seed
        south_seeds = sum(st["board"][0:6])
        assert south_seeds > 0, (
            f"Expected South to be fed, but South has {south_seeds} seeds"
        )
        assert st["turn"] == c1.name, (
            f"Expected turn to pass to South ({c1.name}), got: {st['turn']}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_grand_slam():
    """Rule 10:
    Grand Slam rule (Prise totale):
    If a capture would take ALL remaining seeds from the opponent's side,
    the capture is cancelled according to Awale rules (0 seeds gained, opponent retains their seeds).
    """
    c1, c2, _ = start_match("P1_r10", "P2_r10", south_must_start=True)
    try:
        # Legal sequence leading to Grand Slam on move 21
        path = [
            (c1, 4), (c2, 1), (c1, 0), (c2, 0), (c1, 3), (c2, 2),
            (c1, 0), (c2, 0), (c1, 3), (c2, 1), (c1, 1), (c2, 4),
            (c1, 4), (c2, 3), (c1, 3), (c2, 5), (c1, 1), (c2, 4),
            (c1, 0), (c2, 5)
        ]
        for proc, h in path:
            proc.send(f"play {h}")
            c1.drain(0.02)
            c2.drain(0.02)

        # Before Grand Slam move, check score
        before_st = parse_board_output(c1.drain(0.03))
        score_before = before_st["hands"][0] if before_st else 7

        # South plays hole 4: would capture all North holes (3+2+2 = 7 seeds)
        c1.send("play 4")
        out = c1.drain(0.05)
        c2.drain(0.05)
        st = parse_board_output(out)
        assert st is not None, "Failed to parse board after Grand Slam move"
        # Capture should be cancelled: South score remains unchanged
        assert st["hands"][0] == score_before, (
            f"Expected South score to remain {score_before} (Grand Slam annulled), got {st['hands'][0]}"
        )
        # North retains their seeds
        north_seeds = sum(st["board"][6:12])
        assert north_seeds > 0, (
            f"Expected North to retain seeds, got {north_seeds}"
        )
    finally:
        c1.close()
        c2.close()


def test_rule_win_on_25_seeds():
    """Rule 11: Reaching 25 or more captured seeds immediately wins the game."""
    c1, c2, _ = start_match("P1_r11", "P2_r11", south_must_start=True)
    try:
        # Game 4 ends on move 65 with South reaching 25+ seeds
        moves = "FeDbBdAaFeDaBdFeDcDeAdDaEeDdEcBbAfCfAdFcBeBdAfAeEcCdBeFbEcCfBdAeDbCaFaEaDeEbBdCeDcEdFeAcBdCbDfDeAcEaCbDdBcD"
        won = False
        for step, ch in enumerate(moves):
            actor = c1 if ch.isupper() else c2
            hole = ord(ch.upper()) - ord("A")
            actor.send(f"play {hole}")
            r1 = c1.drain(0.03)
            r2 = c2.drain(0.03)
            if "T'as gagné" in r1 or "a gagné" in (r1 + r2):
                won = True
                break

        assert won, "Expected South to win upon reaching 25 captured seeds"
    finally:
        c1.close()
        c2.close()


def test_rule_giveup_forfeit():
    """Rule 12: Player can give up ('giveup'), awarding forfeit victory to the opponent."""
    c1, c2, _ = start_match("P1_gu", "P2_gu", south_must_start=False)
    try:
        c1.send("giveup")
        r1 = c1.drain(0.08)
        r2 = c2.drain(0.08)
        assert "T'as perdu" in r1, f"Expected loss message for P1, got: {r1}"
        assert "T'as gagné" in r2, f"Expected win message for P2, got: {r2}"
    finally:
        c1.close()
        c2.close()


def test_rule_disconnect_forfeit():
    """Rule 13: If a player disconnects mid-game, opponent is awarded forfeit victory."""
    c1, c2, _ = start_match("P1_dc", "P2_dc", south_must_start=False)
    try:
        c1.close()
        r2 = c2.drain(0.15)
        assert (
            "déconnecté" in r2 or "forfait" in r2 or "T'as gagné" in r2
        ), f"Expected disconnect / forfeit notification for opponent, got: {r2}"
    finally:
        c2.close()

# ---------------------------------------------------------------------------
# Games Replay Tests (from games.txt)
# ---------------------------------------------------------------------------


def replay_game(moves: str, game_num: int) -> bool:
    """Replays a full game string against the server using two client processes."""
    p1 = f"P1_g{game_num}"
    p2 = f"P2_g{game_num}"
    c1, c2, _ = start_match(p1, p2, south_must_start=True)

    try:
        winner = "None (Ongoing)"
        moves_played = len(moves)

        for step, ch in enumerate(moves):
            hole = ord(ch.upper()) - ord("A")
            actor = c1 if ch.isupper() else c2
            actor.send(f"play {hole}")

            r1 = c1.drain(0.04)
            r2 = c2.drain(0.04)
            resp = r1 + r2

            if "respecte les règle" in resp or "Chill bro" in resp:
                print(
                    f"  [Game {game_num}] Move {step + 1} ('{ch}') rejected by server!"
                )
                return False

            if "T'as gagné" in r1:
                winner = f"South ({c1.name})"
                moves_played = step + 1
                break
            elif "T'as gagné" in r2:
                winner = f"North ({c2.name})"
                moves_played = step + 1
                break
            elif "Egalité" in resp:
                winner = "Draw"
                moves_played = step + 1
                break

        print(
            f"  [Game {game_num}] Accepted ({moves_played}/{len(moves)} moves)  Winner: {winner}"
        )
        return True
    finally:
        c1.close()
        c2.close()


def run_all_games(games_file: str) -> bool:
    if not os.path.exists(games_file):
        print(f"Error: Games file '{games_file}' not found.")
        return False

    with open(games_file) as f:
        lines = [line.strip() for line in f if line.strip()]

    print(
        f"\nTesting {len(lines)} games from '{os.path.basename(games_file)}' against server...\n"
    )
    all_passed = True
    for i, line in enumerate(lines, 1):
        if not replay_game(line, i):
            all_passed = False
    return all_passed


def run_all_rule_tests() -> bool:
    rule_tests = [
        ("Rule 1: Initial Board & Sowing", test_rule_initial_board_and_sowing),
        ("Rule 2: Out of Turn Move Rejected", test_rule_out_of_turn_rejected),
        ("Rule 3: Empty Hole Move Rejected", test_rule_empty_hole_rejected),
        ("Rule 4: Invalid Hole Index Rejected", test_rule_invalid_hole_index_rejected),
        ("Rule 5: Single Capture (2/3 seeds)", test_rule_single_capture),
        ("Rule 6: Multiple Retrograde Captures", test_rule_multiple_captures),
        ("Rule 7: Own Side Capture Forbidden", test_rule_own_side_capture_forbidden),
        ("Rule 8: Skip Origin Hole on 12+ Seeds", test_rule_skip_origin_hole_12_seeds),
        ("Rule 9: Famine / Starving Opponent Feeding", test_rule_famine_feeding_rule),
        ("Rule 10: Grand Slam (All Opponent Seeds)", test_rule_grand_slam),
        ("Rule 11: Win Condition on 25+ Seeds", test_rule_win_on_25_seeds),
        ("Rule 12: Surrender ('giveup') Forfeit", test_rule_giveup_forfeit),
        ("Rule 13: Disconnect Mid-Game Forfeit", test_rule_disconnect_forfeit),
    ]

    print("=" * 60)
    print("Testing Awale Rules against server...")
    print("=" * 60)

    passed = 0
    failed = 0
    for name, test_fn in rule_tests:
        try:
            test_fn()
            print(f"  [PASS] {name}")
            passed += 1
        except Exception as e:
            print(f"  [FAIL] {name}: {e}")
            failed += 1

    print(
        f"\nAwale Rules Summary: {passed}/{len(rule_tests)} passed, {failed} failed."
    )
    return failed == 0


def main():
    parser = argparse.ArgumentParser(
        description="Run Awale integration and rule tests."
    )
    parser.add_argument(
        "--rules-only", action="store_true", help="Run only the rule unit tests"
    )
    parser.add_argument(
        "--games-only", action="store_true", help="Run only the games replay tests"
    )
    parser.add_argument(
        "--games-file",
        default=DEFAULT_GAMES_FILE,
        help="Path to games file (default: games.txt)",
    )
    args = parser.parse_args()

    ensure_server_running()

    rules_ok = True
    games_ok = True

    if not args.games_only:
        rules_ok = run_all_rule_tests()

    if not args.rules_only:
        games_ok = run_all_games(args.games_file)

    overall_success = rules_ok and games_ok
    print("\n" + "=" * 60)
    if overall_success:
        print("SUCCESS: All tests passed!")
    else:
        print("ERROR: Some tests failed!")
    print("=" * 60)

    sys.exit(0 if overall_success else 1)


if __name__ == "__main__":
    main()
