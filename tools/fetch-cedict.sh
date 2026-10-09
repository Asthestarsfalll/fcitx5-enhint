#!/usr/bin/env bash
# Download the CC-CEDICT dictionary used by fcitx5-enhint.
#
# Usage:
#   tools/fetch-cedict.sh [output-directory]
#
# The default output directory is ~/.local/share/fcitx5/enhint, where the
# addon looks for cedict_ts.u8.
set -euo pipefail

url="https://www.mdbg.net/chinese/export/cedict/cedict_1_0_ts_utf-8_mdbg.txt.gz"
out_dir="${1:-${XDG_DATA_HOME:-$HOME/.local/share}/fcitx5/enhint}"
out_file="$out_dir/cedict_ts.u8"

mkdir -p "$out_dir"
echo "Downloading CC-CEDICT ..."
tmp="$(mktemp)"
trap 'rm -f "$tmp"' EXIT

if command -v curl >/dev/null 2>&1; then
    curl -L --fail -o "$tmp" "$url"
else
    wget -O "$tmp" "$url"
fi

gzip -dc "$tmp" > "$out_file"
echo "Wrote $out_file ($(wc -l < "$out_file") lines)"
