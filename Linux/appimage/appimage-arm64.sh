#!/bin/bash

export PUBLISHFLAGS="-r linux-arm64 --self-contained true -p:PublishSingleFile=true -p:IncludeNativeLibrariesForSelfExtract=true -p:PublishReadyToRun=true"
# Use clang-18 for C++23 support on ubuntu-22.04 with libc++ for <format>
make -j$(nproc) -O LTO=true STATICLINK=true SYSTEM_LIBEVDEV=false CC=clang-18 CXX=clang++-18 EXTRA_CXXFLAGS="-stdlib=libc++"

curl -SL https://github.com/AppImage/AppImageKit/releases/download/continuous/appimagetool-aarch64.AppImage -o appimagetool

mkdir -p AppDir/usr/bin
cp bin/linux-arm64/Release/linux-arm64/publish/Nexen AppDir/usr/bin
chmod +x AppDir/usr/bin
ln -sr AppDir/usr/bin/Nexen AppDir/AppRun

cp Linux/appimage/Nexen.48x48.png AppDir/Nexen.png
cp Linux/appimage/Nexen.desktop AppDir/Nexen.desktop
mkdir -p AppDir/usr/share/applications && cp ./AppDir/Nexen.desktop ./AppDir/usr/share/applications
mkdir -p AppDir/usr/share/icons && cp ./AppDir/Nexen.png ./AppDir/usr/share/icons
mkdir -p AppDir/usr/share/icons/hicolor/48x48/apps && cp ./AppDir/Nexen.png ./AppDir/usr/share/icons/hicolor/48x48/apps

chmod a+x appimagetool
ARCH=aarch64 ./appimagetool AppDir/ Nexen.AppImage
