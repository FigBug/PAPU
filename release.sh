#!/bin/bash -e
set -x

cd "$(dirname "$0")"
ROOT=$(pwd)

TAG="$GITHUB_REF_NAME"
# Tags are pushed with a leading "v" (see tag.sh) but Changelist entries are
# numeric. Use TAG for the gh release tag, VER (stripped) elsewhere.
VER="${TAG#v}"

NOTES=$(awk -v ver="$VER" '
    BEGIN { found=0; printing=0; pattern="^"ver":?$" }
    $0 ~ pattern { found=1; printing=1; next }
    printing && /^[0-9]+\.[0-9]+\.[0-9]+:?$/ { printing=0 }
    printing { print }
    END { if (!found) exit 1 }
' ./Changelist.txt)

if [ $? -ne 0 ] || [ -z "$NOTES" ]; then
    echo "Error: Version $VER not found in Changelist.txt or has no content"
    exit 1
fi

echo "$NOTES" > /tmp/release_notes.txt

ASSETS=(
  "./Binaries Linux"/*.deb
  "./Binaries Windows"/*.exe
  "./Binaries macOS"/*.pkg
)
if [ -f "./Binaries macOS/Symbols_Mac.zip" ]; then
  ASSETS+=("./Binaries macOS/Symbols_Mac.zip")
fi

gh release create "$TAG" --title "$TAG" -F /tmp/release_notes.txt "${ASSETS[@]}"

PLUGIN=papu
for f in "./Binaries Linux"/*.deb \
         "./Binaries Windows"/*.exe \
         "./Binaries macOS"/*.pkg; do
  curl -sS --fail-with-body -F "files=@${f}" \
          -F "plugin=${PLUGIN}" \
          -F "version=${VER}" \
          -F "changelog=${NOTES}" \
          "https://socalabs.com/files/upload.php?key=$APIKEY"
done
