**Build instructions**

GeoDa is built from the repository root with CMake (`CMakeLists.txt`); the
macOS CI workflow is `.github/workflows/osx_build.yml`.

Files here support that build:

- `bundle_dylibs.py` — bundles non-system dylibs into `Contents/Frameworks`
  (run as a POST_BUILD step by CMake)
- `create-dmg/` — DMG packaging
- `GeoDa.icns` — application icon
