#!/bin/bash

# mac specific stuff
if [ $OS = "mac" ]; then
  # Create a temp keychain
  if [ -n "$GITHUB_ACTIONS" ]; then
    echo "Create a keychain"
    security create-keychain -p nr4aGPyz Keys.keychain

    echo $APPLICATION | base64 -D -o /tmp/Application.p12
    echo $INSTALLER | base64 -D -o /tmp/Installer.p12

    security import /tmp/Application.p12 -t agg -k Keys.keychain -P aym9PKWB -A -T /usr/bin/codesign
    security import /tmp/Installer.p12 -t agg -k Keys.keychain -P aym9PKWB -A -T /usr/bin/codesign

    security list-keychains -s Keys.keychain
    security default-keychain -s Keys.keychain
    security unlock-keychain -p nr4aGPyz Keys.keychain
    security set-keychain-settings -l -u -t 13600 Keys.keychain
    security set-key-partition-list -S apple-tool:,apple: -s -k nr4aGPyz Keys.keychain
  fi
  DEV_APP_ID="Developer ID Application: Roland Rabien (3FS7DJDG38)"
  DEV_INST_ID="Developer ID Installer: Roland Rabien (3FS7DJDG38)"
else
  # windows stuff
fi

ROOT=$(cd "$(dirname "$0")/.."; pwd)
cd "$ROOT"
echo "$ROOT"

cd "$ROOT/ci"
rm -Rf bin
mkdir bin

# Get the hash
cd "$ROOT/modules/juce"
HASH=`git rev-parse HEAD`
echo "Hash: $HASH"

# Get the Projucer
cd "$ROOT/ci/bin"
while true
do
  PROJUCER_URL=`wget https://projucer.rabien.com/get_projucer.php?hash=$HASH&os=$OS&key=$APIKEY`
  echo $PROJUCER_URL
  if [ $PROJUCER_URL == http* ]; then
    wget $PROJUCER_URL
    unzip Projucer.zip
    break
  fi
  sleep 1
done

# Resave jucer file
if [ "$OS" = "mac" ]; then
  "$ROOT/ci/bin/Projucer.app/Contents/MacOS/Projucer" --resave "$ROOT/plugin/PAPU.jucer"
else
  "$ROOT/ci/bin/Projucer.exe" --resave "$ROOT/plugin/PAPU.jucer"
fi

# Build mac version
if [ "$OS" = "mac" ]; then
  cd "$ROOT/plugin/Builds/MacOSX"
  xcodebuild -configuration Release || exit 1

  cp -R ~/Library/Audio/Plug-Ins/VST/PAPU.vst "$ROOT/ci/bin"
  cp -R ~/Library/Audio/Plug-Ins/Components/PAPU.component "$ROOT/ci/bin"

  cd "$ROOT/ci/bin"
  for filename in ./*.vst; do
    codesign -s "$DEV_APP_ID" -v "$filename"
  done
  for filename in ./*.component; do
    codesign -s "$DEV_APP_ID" -v "$filename" 
  done

  cd "$ROOT/ci/bin"
  zip -r PAPU_Mac.zip PAPU.vst PAPU.component
fi

