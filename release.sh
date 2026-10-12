#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
VER=$(sed -n 's/^#define FATRAINER_VERSION "\(.*\)"/\1/p' src/version.h)
NAME=$(sed -n 's/^#define FATRAINER_NAME "\(.*\)"/\1/p' src/version.h)
SLUG=${VER// /-}
HEADING="## $VER"
awk -v v="$HEADING" -v n="$NAME" '$0 == v {getline; getline; found = $0 == n} END {exit !found}' CHANGELOG.md || { echo "CHANGELOG.md has no $HEADING section named $NAME"; exit 1; }
./build.sh
./build_installer.sh
OUT=dist/release
mkdir -p "$OUT"
rm -f "$OUT/xinput1_3.dll" "$OUT/xinput1_3-visuals.dll" "$OUT/CHANGELOG.md" "$OUT/version.txt" "$OUT/version-visuals.txt" "$OUT/notes.md"
cp xinput1_3.dll xinput1_3-visuals.dll CHANGELOG.md "$OUT/"
LEGACY_FINAL_BUILD=999
if [[ "$VER" == *" b"* ]]; then
  printf 'version %s\nbuild %s\n' "${VER%% *}" "$VER" > "$OUT/version.txt"
else
  printf 'version %s\nbuild %s b%s\nrelease %s\n' "$VER" "$VER" "$LEGACY_FINAL_BUILD" "$VER" > "$OUT/version.txt"
fi
grep -v '^sha256\|^size' "$OUT/version.txt" > "$OUT/version-visuals.txt" || true
printf 'sha256 %s\nsize %s\n' "$(sha256sum xinput1_3.dll | cut -d' ' -f1)" "$(stat -c%s xinput1_3.dll)" >> "$OUT/version.txt"
printf 'sha256 %s\nsize %s\n' "$(sha256sum xinput1_3-visuals.dll | cut -d' ' -f1)" "$(stat -c%s xinput1_3-visuals.dll)" >> "$OUT/version-visuals.txt"
mkdir -p nexus
python3 - "$VER" "nexus/FaTrainer-Visuals-$SLUG.zip" <<'PY'
import sys, zipfile
version, target = sys.argv[1], sys.argv[2]
readme = f"""FaTrainer | Dying Light {version}, Visuals Version

Changes only how Dying Light looks to you: your own UV light color, Night Hunter glow color,
HUD colors and game texts. No cheats, nothing that changes gameplay, no internet connection.

Install
1. Put xinput1_3.dll next to DyingLightGame.exe in the Dying Light folder.
2. Linux (Steam with Proton): set the launch options to
   WINEDLLOVERRIDES="xinput1_3=n,b" %command%
3. Start the game and press Insert or F8 for the menu.

Remove: delete xinput1_3.dll from the game folder.
Settings are saved in fatrainer.ini next to the game.
"""
with zipfile.ZipFile(target, "w", zipfile.ZIP_DEFLATED) as z:
    z.write("xinput1_3-visuals.dll", "xinput1_3.dll")
    z.writestr("readme.txt", readme)
PY
awk -v v="$HEADING" '$0 == v {on = 1; next} /^## / {on = 0} on' CHANGELOG.md > "$OUT/notes.md"
if [ "${1:-}" = "--beta" ]; then
  nix-shell -p gh --run "
    gh release view beta >/dev/null 2>&1 || gh release create beta --title 'FaTrainer beta $VER · $NAME' --notes-file $OUT/notes.md --prerelease --latest=false
    gh release upload beta $OUT/xinput1_3.dll $OUT/version.txt $OUT/xinput1_3-visuals.dll $OUT/version-visuals.txt $OUT/CHANGELOG.md --clobber
    gh release edit beta --title 'FaTrainer beta $VER · $NAME' --notes-file $OUT/notes.md --prerelease --latest=false"
fi
if [ "${1:-}" = "--publish" ]; then
  nix-shell -p gh --run "gh release create v$SLUG $OUT/xinput1_3.dll $OUT/version.txt $OUT/xinput1_3-visuals.dll $OUT/version-visuals.txt $OUT/CHANGELOG.md --title 'FaTrainer $VER · $NAME' --notes-file $OUT/notes.md --latest"
fi
echo "release files: $OUT"
