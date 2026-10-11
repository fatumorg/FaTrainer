#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
VER=$(sed -n 's/^#define FATRAINER_VERSION "\(.*\)"/\1/p' src/version.h)
SLUG=${VER// /-}
grep -q "^## $VER\$" CHANGELOG.md || { echo "CHANGELOG.md has no ## $VER section"; exit 1; }
./build.sh
./build_installer.sh
OUT=dist/release
mkdir -p "$OUT"
rm -f "$OUT/xinput1_3.dll" "$OUT/CHANGELOG.md" "$OUT/version.txt" "$OUT/notes.md"
cp xinput1_3.dll CHANGELOG.md "$OUT/"
LEGACY_FINAL_BUILD=999
if [[ "$VER" == *" b"* ]]; then
  printf 'version %s\nbuild %s\n' "${VER%% *}" "$VER" > "$OUT/version.txt"
else
  printf 'version %s\nbuild %s b%s\nrelease %s\n' "$VER" "$VER" "$LEGACY_FINAL_BUILD" "$VER" > "$OUT/version.txt"
fi
printf 'sha256 %s\nsize %s\n' "$(sha256sum xinput1_3.dll | cut -d' ' -f1)" "$(stat -c%s xinput1_3.dll)" >> "$OUT/version.txt"
awk -v v="## $VER" '$0 == v {on = 1; next} /^## / {on = 0} on' CHANGELOG.md > "$OUT/notes.md"
if [ "${1:-}" = "--beta" ]; then
  nix-shell -p gh --run "
    gh release view beta >/dev/null 2>&1 || gh release create beta --title 'FaTrainer beta $VER' --notes-file $OUT/notes.md --prerelease --latest=false
    gh release upload beta $OUT/xinput1_3.dll $OUT/version.txt $OUT/CHANGELOG.md --clobber
    gh release edit beta --title 'FaTrainer beta $VER' --notes-file $OUT/notes.md --prerelease --latest=false"
fi
if [ "${1:-}" = "--publish" ]; then
  nix-shell -p gh --run "gh release create v$SLUG $OUT/xinput1_3.dll $OUT/version.txt $OUT/CHANGELOG.md --title 'FaTrainer $VER' --notes-file $OUT/notes.md --latest"
fi
echo "release files: $OUT"
