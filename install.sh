#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
GAME="${1:-/data/Gaming/Steam/steamapps/common/Dying Light}"
[ -f "$GAME/DyingLightGame.exe" ] || { echo "Dying Light not found at: $GAME (pass the folder as argument)"; exit 1; }
[ -f xinput1_3.dll ] || ./build.sh
cp xinput1_3.dll "$GAME/xinput1_3.dll.new"
mv -f "$GAME/xinput1_3.dll.new" "$GAME/xinput1_3.dll"
echo "Installed FaTrainer to $GAME/xinput1_3.dll"
echo
echo "Steam > Dying Light > Properties > Launch Options must include xinput1_3=n,b in WINEDLLOVERRIDES, e.g."
echo '  WINEDLLOVERRIDES="xinput1_3=n,b" %command%'
echo
echo "In game press Insert or F8. Log: $GAME/fatrainer.log"
