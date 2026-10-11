#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
if [ "${1:-}" = "--clear" ]; then
  git rm -q announcement.txt
  git commit -q -m "Announcement removed" && git push -q origin main
  echo "announcement removed"
  exit 0
fi
[ $# -eq 2 ] || { echo 'usage: ./announce.sh "Title" "Text"  |  ./announce.sh --clear'; exit 1; }
printf 'id=%s\ntitle=%s\n\n%s\n' "$(date -u +%Y%m%d%H%M%S)" "$1" "$2" > announcement.txt
git add announcement.txt
git commit -q -m "Announcement: $1" && git push -q origin main
echo "posted: the FaTrainer launcher shows it once on its next start"
