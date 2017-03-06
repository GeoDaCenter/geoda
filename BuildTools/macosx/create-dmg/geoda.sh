#!/bin/sh

VERSION=$1

./create-dmg --volname "GeoDa $VERSION Installer" --volicon "GeoDa_installer.icns" --window-pos 200 120 --window-size 800 400 --icon-size 100 --icon GeoDa.app 200 190 --hide-extension GeoDa.app --background "geoda_installer_bg.png" --app-drop-link 600 185 GeoDa$VERSION-Installer.dmg ~/geoda_trunk/BuildTools/macosx/build/
