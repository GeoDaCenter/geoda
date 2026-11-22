**Build instructions**

GeoDa is built from the repository root with CMake (`CMakeLists.txt`). The
Windows dependencies (wxWidgets, GDAL via vcpkg, Boost, CLAPACK, Eigen, OpenCL)
and the Inno Setup installers are prepared by
`.github/workflows/windows_build.yml`; `deps.md` explains how those
dependencies are built.

The remaining files here are inputs to that workflow:

- `dep/` — patches/project files used while building the dependencies
- `installer/` — Inno Setup scripts
- `update_installer_version.py` — stamps the version from `version.h`
