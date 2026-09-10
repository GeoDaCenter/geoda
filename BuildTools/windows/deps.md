# Dependencies

boost 1.75
wxWidgets 3.1.4
JSON Spirit v4.08
CLAPACK 3.1.1
Eigen3
Spectra v0.8.0
OpenCL

gdal / curl / zlib: built from source with vcpkg, pinned to the VCPKG_COMMIT in
.github/workflows/windows_build.yml for reproducible builds. Modern GDAL with
the Parquet and Arrow OGR drivers in the x64 build; the old gisinternals
release-1911 SDK was GDAL 3.0.4 (2019) and had no Parquet/Arrow drivers. See
.github/workflows/windows_build.yml for the per-architecture feature set
(note: the valid feature names come from ports/gdal/vcpkg.json of the pinned
commit -- e.g. the LZMA feature is `lzma`, and there is no `odbc` feature).

Driver coverage vs the old SDK:
- New: Parquet, Arrow (x64 only; Apache Arrow C++ does not build for 32-bit
  MSVC, so x86 is built without those two features).
- Dropped: the old SDK's FileGDB (Esri), MSSQLSpatial and OCI plugin DLLs are
  not produced by vcpkg. The PostgreSQL driver is compiled into gdal.dll via
  the `postgresql` feature.

Boost shadowing: the GDAL `libkml` feature pulls the vcpkg boost metaport into
the installed tree, and its boost-uninstall port installs
`share/boost/vcpkg-cmake-wrapper.cmake` + `share/boost/BoostConfig.cmake`.
That wrapper forces `Boost_NO_BOOST_CMAKE=OFF` and routes `find_package(Boost)`
to the vcpkg BoostConfig, which does per-component `find_package(boost_<comp>)`
and fails because only libkml's minimal boost subset is installed (no
boost_thread, etc.). GeoDa links its own static Boost 1.75 (temp/boost,
-vc142), so the "Build GeoDa" step deletes
`installed/<triplet>/share/boost` and `installed/<triplet>/include/boost`
before configuring; with include/boost gone the vcpkg toolchain's special Boost
branch is skipped and find_package(Boost) falls through to the classic
FindBoost module using -DBoost_ROOT/-DBoost_LIBRARY_DIR/-DBoost_COMPILER=-vc142.

Minimum OS: vcpkg builds with the VS2022 (v143) toolset targeting the current
Windows SDK, so the bundled GDAL/Arrow/curl DLLs and GeoDa.exe require
Windows 8.1 or newer -- the "win8+" installers (installer/*bit/GeoDa-win8+.iss)
do NOT run on Windows 7.

First cold build compiles the full GDAL dependency tree (geos, hdf5, netcdf,
Arrow, ...) from source and can take 2-4h on a 2-core GitHub runner; warm runs
restore the vcpkg tree and binary cache from actions/cache. The bundled
CommonDistFiles/proj/proj.db is an older EPSG dataset (2019); the vcpkg-built
PROJ reads it, but if PROJ ever complains about the database schema, regenerate
it from the vcpkg proj data (installed/<triplet>/share/proj) or bump it.