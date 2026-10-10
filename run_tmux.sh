#!/usr/bin/env bash
set -euo pipefail

# this script was not the aim of the project and thus was not coded by hand <!> but mainly by copy pasting on the web and vibe coding it

# ---------------------------------------------------------------------------
# run_tmux.sh
# 
# Builds the project and starts a tmux session with 1 server and N clients:
#   - Server starts first.
#   - Clients wait 'sleep 1' before launching.
#   - Each client is named bob1, bob2, ..., bobN.
#   - When ANY tab (or process) disconnects/exits, EVERYONE quits immediately.
# ---------------------------------------------------------------------------

DEFAULT_N=2
SESSION_NAME="${SESSION_NAME:-tp_chat}"
HOST="${HOST:-127.0.0.1}"
N=""

show_help() {
    cat <<EOF
Usage: $(basename "$0") [N] [OPTIONS]

Arguments:
  N                  Number of clients (default: ${DEFAULT_N})

Options:
    --session <name>   Tmux session name (default: ${SESSION_NAME})
  -h, --help         Show this help message

Makefile target:
  make run           Starts tmux with default N=2 clients
  make run N=3       Starts tmux with 3 clients
  make tmux N=4      Alias for make run

When any tab/client/server disconnects or closes (e.g. Ctrl+C or exit),
the entire tmux session is automatically killed.
EOF
    exit 0
}

# Parse command-line arguments
while [[ $# -gt 0 ]]; do
    case "$1" in
        -h|--help)
            show_help
            ;;
        --host)
            HOST="$2"
            shift 2
            ;;
        --session)
            SESSION_NAME="$2"
            shift 2
            ;;
        *)
            if [[ -z "$N" && "$1" =~ ^[0-9]+$ ]]; then
                N="$1"
                shift
            else
                echo "Error: Unrecognized option or invalid number of clients: $1" >&2
                echo "Run '$(basename "$0") --help' for usage." >&2
                exit 1
            fi
            ;;
    esac
done

N="${N:-${CLIENTS:-$DEFAULT_N}}"

if ! [[ "$N" =~ ^[1-9][0-9]*$ ]]; then
    echo "Error: Number of clients N must be a positive integer >= 1 (got '$N')." >&2
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

SERVER_BIN="./build/server"
CLIENT_BIN="./build/client"

if ! command -v tmux >/dev/null 2>&1; then
    echo "install tmux"
    exit 1
fi

# 3. Clean up any existing session with the same name
tmux kill-session -t "$SESSION_NAME" 2>/dev/null || true

# Session kill command to trigger whenever any process terminates
KILL_CMD="tmux kill-session -t '$SESSION_NAME' 2>/dev/null"

echo "==> Starting tmux session '$SESSION_NAME' with 1 server and $N client(s) (mode)..."

# -----------------------------------------------------------------------
# PANE MODE: All clients and the server tiled in a single window.
# -----------------------------------------------------------------------
tmux new-session -d -s "$SESSION_NAME" -n "main" \
        "bash -c 'trap \"$KILL_CMD\" EXIT INT TERM; $SERVER_BIN; $KILL_CMD'"

tmux set-hook -t "$SESSION_NAME" pane-exited "kill-session -t '$SESSION_NAME'"
tmux set-hook -t "$SESSION_NAME" window-unlinked "kill-session -t '$SESSION_NAME'"
tmux set-option -t "$SESSION_NAME" mouse on 2>/dev/null || true
for i in $(seq 1 "$N"); do
    CLIENT_NAME="bob$i"
    tmux split-window -t "$SESSION_NAME:main" \
        "bash -c 'trap \"$KILL_CMD\" EXIT INT TERM; sleep 1; $CLIENT_BIN $HOST $CLIENT_NAME; $KILL_CMD'"
    tmux select-layout -t "$SESSION_NAME:main" tiled
done

tmux select-pane -t "$SESSION_NAME:main.1"

# 4. Attach or switch to the tmux session
if [[ -t 0 && -t 1 && "${TERM:-}" != "dumb" && -n "${TERM:-}" ]]; then
    if [[ -n "${TMUX:-}" ]]; then
        tmux switch-client -t "$SESSION_NAME"
    else
        tmux attach-session -t "$SESSION_NAME"
    fi
else
    echo "==> Session '$SESSION_NAME' started in background."
    echo "    Attach with: tmux attach-session -t $SESSION_NAME"
fi