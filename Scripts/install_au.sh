#!/bin/bash
set -euo pipefail
repo="$(cd "$(dirname "$0")/.." && pwd)"
source="${1:-}"
if [ -z "$source" ]; then
    path_file="$repo/build/au-bundle-path.txt"
    [ -f "$path_file" ] || { echo "Build the AU first." >&2; exit 1; }
    source="$(cat "$path_file")"
fi
destination="${2:-$HOME/Library/Audio/Plug-Ins/Components}"
target="$destination/Hendrix Guitar.component"
[ -d "$source/Contents/Resources/Guitar" ] && [ -f "$source/Contents/Resources/Guitar/mapping.json" ] || { echo "Missing built guitar bundle: $source" >&2; exit 1; }
[ ! -L "$target" ] || { echo "Installed component is a symbolic link; refusing to replace it." >&2; exit 1; }
/usr/bin/codesign --verify --deep --strict "$source"
mkdir -p "$destination"
stage="$(mktemp -d "$destination/.hendrix-install-XXXXXX")"
trap 'rm -rf "$stage"' EXIT
candidate="$stage/Hendrix Guitar.component"
/usr/bin/ditto --norsrc "$source" "$candidate"
/usr/bin/codesign --verify --deep --strict "$candidate"
if [ -e "$target" ]; then
    backups="$(dirname "$destination")/HendrixGuitar Backups"
    mkdir -p "$backups"
    archive="$backups/Hendrix Guitar-$(date +%Y%m%d-%H%M%S)-$(basename "$stage").zip"
    /usr/bin/ditto -c -k --sequesterRsrc --keepParent "$target" "$archive"
    mv "$target" "$stage/previous.component"
    echo "Previous copy backed up: $archive"
fi
if mv "$candidate" "$target"; then
    echo "Installed: $target"
else
    [ ! -d "$stage/previous.component" ] || mv "$stage/previous.component" "$target"
    exit 1
fi
