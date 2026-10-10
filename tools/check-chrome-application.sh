#!/usr/bin/env bash
# Explicit real-browser acceptance: requires installed Google Chrome and a display.
set -euo pipefail
ROOT="$(cd "$(dirname "$0")/.." && pwd)"
GODOT="${GODOT_BINARY:-$ROOT/tools/Godot_v4.7.2-stable_linux.x86_64}"
"$ROOT/tools/validate-godot-project.sh"
export ELSEWHERE_LAUNCHER_STATE_DIR="$ROOT/.tools/chrome-launcher-check"
LOG="$ROOT/.tools/chrome-application-check.log"
if ! timeout -k 2s 100s "$GODOT" --path "$ROOT/world" --audio-driver Dummy --max-fps 60 --script res://tests/chrome_application.gd > "$LOG" 2>&1; then
    cat "$LOG"
    exit 1
fi
cat "$LOG"
if rg -n 'ERROR:|crashed with signal|Leaked instance|ObjectDB instances leaked' "$LOG" || ! rg -q '^CHROME_APPLICATION_OK failures=0$' "$LOG"; then
    echo 'Chrome application check failed' >&2
    exit 1
fi
echo "Real Chrome checks passed; captures: $ROOT/.tools/chrome-application-test"
