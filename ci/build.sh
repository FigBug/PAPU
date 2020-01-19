#!/bin/bash -e
set -x

set

PLUGIN="PAPU"

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
  PROJUCER_URL=$(curl -s -S "https://projucer.rabien.com/get_projucer.php?hash=$HASH&os=$OS&key=$APIKEY")
  echo "Response: $PROJUCER_URL"
  if [[ $PROJUCER_URL == http* ]]; then
    curl -s -S $PROJUCER_URL -o "$ROOT/ci/bin/Projucer.zip"
    unzip Projucer.zip
    break
  fi
  sleep 15
done

# Resave jucer file
if [ "$OS" = "mac" ]; then
  "$ROOT/ci/bin/Projucer.app/Contents/MacOS/Projucer" --resave "$ROOT/plugin/$PLUGIN.jucer"
else
  "$ROOT/ci/bin/Projucer.exe" --resave "$ROOT/plugin/$PLUGIN.jucer"
fi

# Build mac version
if [ "$OS" = "mac" ]; then
  cd "$ROOT/plugin/Builds/MacOSX"
  xcodebuild -configuration Release || exit 1

  cp -R ~/Library/Audio/Plug-Ins/VST/$PLUGIN.vst "$ROOT/ci/bin"
  cp -R ~/Library/Audio/Plug-Ins/Components/$PLUGIN.component "$ROOT/ci/bin"

  cd "$ROOT/ci/bin"
  for filename in ./*.vst; do
    codesign -s "$DEV_APP_ID" -v "$filename" --options=runtime
  done
  for filename in ./*.component; do
    codesign -s "$DEV_APP_ID" -v "$filename" --options=runtime
  done

  cd "$ROOT/ci/bin"
  zip -r $PLUGIN_Mac.zip $PLUGIN.vst $PLUGIN.component
fi

# Build Win version
if [ "$OS" = "win" ]; then
  VS_WHERE="C:/Program Files (x86)/Microsoft Visual Studio/Installer/vswhere.exe"
  
  MSBUILD_EXE=$("$VS_WHERE" -latest -requires Microsoft.Component.MSBuild -find "MSBuild\**\Bin\MSBuild.exe")
  echo $MSBUILD_EXE

  cd "$ROOT/plugin/Builds/VisualStudio2017"
  "$MSBUILD_EXE" $PLUGIN.sln "/p:VisualStudioVersion=15.0" /m "/t:Build" "/p:Configuration=Release64" "/p:Platform=x64" "/p:PreferredToolArchitecture=x64"
  "$MSBUILD_EXE" $PLUGIN.sln "/p:VisualStudioVersion=15.0" /m "/t:Build" "/p:Configuration=Release" "/p:PlatformTarget=x86" "/p:PreferredToolArchitecture=x64"

  cd "$ROOT%/Scripts/bin"

  cp "$ROOT%/plugin/Builds/VisualStudio2017/x64/Release64/VST/$PLUGIN_64b.dll"
  cp "$ROOT%/plugin/Builds/VisualStudio2017/Win32/Release/VST/$PLUGIN_32b.dll"

  zip -r $PLUGIN_Win.zip ${PLUGIN}_64b.vst ${PLUGIN}_32b.vst
fi