#!/usr/bin/env bash
# Rejoue un fondu enregistre par emotion-emulator dans l'anneau, et lit avec la sonde.
#   ./outils/rejouer.sh ~/.cache/emotion-emulator/relais/A--B.pak [secondes]
set -uo pipefail
cd "$(dirname "$0")/.."
PAK="${1:?fichier .pak}"; SECONDES="${2:-10}"
EE="${EMOTION_EMULATOR:-$HOME/Documents/emotion-emulator}"
./construire.sh > /dev/null || exit 1
for pid in $(pgrep -f Emotion.Server); do kill "$pid" 2>/dev/null; done
dotnet run -c Release --no-build --project "$EE/tools/Emotion.Probe" -- rejoue "$PAK" 1 > /dev/null &
REJEU=$!
trap 'kill $REJEU 2>/dev/null; pkill -f "Emotion.Probe.*rejoue" 2>/dev/null' EXIT INT TERM
sleep 1.5
./bin/emotion-renderer --sonde "$SECONDES"
