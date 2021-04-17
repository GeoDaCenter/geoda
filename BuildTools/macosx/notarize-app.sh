xcrun altool --notarize-app --primary-bundle-id "edu.uchicago.spatial" --username xunli@uchicago.edu --password "@keychain:AC" --file create-dmg/GeoDa$1-Installer.dmg
# xcrun altool --notarization-history 0 -u xunli@uchicago.edu -p "@keychain:AC"
# xcrun altool --notarization-info 41c63fbc-f238-4874-bc74-6429126f3821 -u xunli@uchicago.edu
# xcrun stapler staple "create-dmg/GeoDa$1-Installer.dmg"
# security find-identity -v