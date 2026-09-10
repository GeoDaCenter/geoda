# Cross-platform build for GeoDa.
#
# Replaces the legacy GNUmake build (root GNUmakefile + GeoDamake.*.opt +
# per-directory GNUmakefiles) and the checked-in MSVC project. Builds against
# system packages on macOS / Linux / Windows plus a few small dependencies
# fetched at configure time.
#
# Prerequisites (installed outside CMake):
#   * wxWidgets 3.x (base core net xml html adv aui propgrid richtext ribbon
#     stc gl xrc qa webview components)
#   * GDAL (>= 3)
#   * Boost (>= 1.66: thread, date_time, system, chrono, atomic, regex)
#   * Eigen3 (>= 3.3; not version-pinned in find_package so it works with both
#     the 3.x series from apt and the 5.x series now shipped by homebrew)
#   * CLAPACK -- see GEODA_CLAPACK_DIR / GEODA_USE_CLAPACK_EXTERNAL below
#
# json_spirit and Spectra are fetched automatically.

cmake_minimum_required(VERSION 3.16)

# Extract fetched archives at configure time (avoids CMP0135 warnings for the
# json_spirit / Spectra / CLAPACK URL downloads).
if(POLICY CMP0135)
  cmake_policy(SET CMP0135 NEW)
endif()

project(geoda VERSION 1.22.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")

option(GEODA_BUILD_TESTS "Build GeoDa unit tests" OFF)
option(GEODA_REGENERATE_XRC "Regenerate rc/GdaAppResources.cpp from the .xrc files via wxrc" OFF)
option(GEODA_USE_CLAPACK_EXTERNAL "Download and build CLAPACK from source (off by default; see GEODA_CLAPACK_PATH)" OFF)

set(GEODA_CLAPACK_PATH "" CACHE PATH
  "Directory containing the prebuilt CLAPACK static libs (lapack.a, blas.a, libf2c.a on Unix; clapack.lib, BLAS.lib, libf2c.lib on Windows)")

# ---------------------------------------------------------------------------
# Dependencies
# ---------------------------------------------------------------------------

include(FetchContent)

find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)      # OpenGL::GL, OpenGL::GLU
find_package(CURL REQUIRED)        # GeoDa code calls libcurl directly
find_package(ZLIB REQUIRED)        # io/MatfileReader.cpp uses zlib
find_package(wxWidgets REQUIRED COMPONENTS
  base core net xml html adv aui propgrid richtext ribbon stc gl xrc qa webview)
find_package(Boost 1.66 REQUIRED COMPONENTS thread date_time chrono atomic regex)
find_package(GDAL REQUIRED)        # GDAL::GDAL
# Eigen3: config mode first (brew/apt ship Eigen3Config.cmake); fall back to
# locating the headers directly. The Windows eigen3.zip ships headers only and
# CMake >= 3.30 no longer ships FindEigen3.cmake, so the module fallback is
# gone. EIGEN3_INCLUDE_DIR is passed explicitly by the Windows workflow.
find_package(Eigen3 QUIET NO_MODULE)
if(NOT TARGET Eigen3::Eigen)
  if(NOT EIGEN3_INCLUDE_DIR)
    find_path(EIGEN3_INCLUDE_DIR NAMES Eigen/Core)
  endif()
  if(NOT EIGEN3_INCLUDE_DIR)
    message(FATAL_ERROR "Eigen3 not found. Set EIGEN3_INCLUDE_DIR to the "
      "directory containing Eigen/Core.")
  endif()
  add_library(Eigen3::Eigen INTERFACE IMPORTED)
  set_target_properties(Eigen3::Eigen PROPERTIES
    INTERFACE_INCLUDE_DIRECTORIES "${EIGEN3_INCLUDE_DIR}")
endif()

# OpenCL is only required on Apple and Windows; the Linux sources stub it out
# themselves (#ifdef __linux__) and do not include any OpenCL header.
if(APPLE OR WIN32)
  find_package(OpenCL REQUIRED)
endif()

if(APPLE)
  # The dylib bundling POST_BUILD step below is a python script.
  find_package(Python3 REQUIRED COMPONENTS Interpreter)
endif()

# json_spirit - small JSON lib with an old CMakeLists that also builds demos/
# tests, so populate the source and build our own static library from it.
FetchContent_Declare(json_spirit
  URL https://github.com/GeoDaCenter/software/releases/download/v2000/json_spirit_v4.08.zip)
FetchContent_GetProperties(json_spirit)
if(NOT json_spirit_POPULATED)
  FetchContent_Populate(json_spirit)
endif()

add_library(json_spirit STATIC
  ${json_spirit_SOURCE_DIR}/json_spirit/json_spirit_reader.cpp
  ${json_spirit_SOURCE_DIR}/json_spirit/json_spirit_value.cpp
  ${json_spirit_SOURCE_DIR}/json_spirit/json_spirit_writer.cpp)
target_include_directories(json_spirit PUBLIC ${json_spirit_SOURCE_DIR})
target_link_libraries(json_spirit PUBLIC Boost::boost)
set_target_properties(json_spirit PROPERTIES POSITION_INDEPENDENT_CODE ON)

# Spectra - header-only eigen solvers; no CMakeLists at v0.8.0.
FetchContent_Declare(spectra
  URL https://github.com/yixuan/spectra/archive/refs/tags/v0.8.0.zip)
FetchContent_GetProperties(spectra)
if(NOT spectra_POPULATED)
  FetchContent_Populate(spectra)
endif()
set(SPECTRA_INCLUDE_DIR "${spectra_SOURCE_DIR}/include")

# ---------------------------------------------------------------------------
# CLAPACK (LAPACK/BLAS/libf2c translated to C)
#
# Not a system package. Two ways to satisfy it:
#   1. GEODA_CLAPACK_PATH  - point at a prebuilt set of static libs.
#      On macOS this defaults to the prebuilt copy in BuildTools/macosx/temp.
#   2. GEODA_USE_CLAPACK_EXTERNAL=ON - download the official CLAPACK 3.2.1
#      tarball and build it (same source build.sh used to run).
# ---------------------------------------------------------------------------
if(GEODA_USE_CLAPACK_EXTERNAL)
  # Portable make.inc - the vendored copies hardcode GEODA_HOME paths. The
  # tarball provides the top-level Makefile (targets f2clib/blaslib/INSTALL/SRC).
  set(GEODA_CLAPACK_SRC_DIR "${CMAKE_BINARY_DIR}/_deps/clapack-src")
  set(GEODA_CLAPACK_MAKEINC "${CMAKE_BINARY_DIR}/clapack-make.inc")
  file(GENERATE OUTPUT "${GEODA_CLAPACK_MAKEINC}" CONTENT
"SHELL = /bin/sh
PLAT =
CC = gcc
CFLAGS = -O3 -Wno-implicit-function-declaration -I$(TOPDIR)/INCLUDE
LOADER = gcc
LOADOPTS =
NOOPT = -O0 -Wno-implicit-function-declaration -I$(TOPDIR)/INCLUDE
DRVCFLAGS = $(CFLAGS)
F2CCFLAGS = $(CFLAGS)
TIMER = INT_CPU_TIME
ARCH = ar
ARCHFLAGS = cr
RANLIB = ranlib
BLASLIB = ../../blas.a
XBLASLIB =
LAPACKLIB = lapack.a
F2CLIB = ../../F2CLIBS/libf2c.a
TMGLIB = tmglib.a
")

  include(ExternalProject)
  ExternalProject_Add(clapack_ext
    URL https://www.netlib.org/clapack/clapack-3.2.1-CMAKE.tgz
    PREFIX "${CMAKE_BINARY_DIR}/_deps/clapack"
    SOURCE_DIR "${GEODA_CLAPACK_SRC_DIR}"
    CONFIGURE_COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${GEODA_CLAPACK_MAKEINC}" "${GEODA_CLAPACK_SRC_DIR}/make.inc"
    BUILD_COMMAND make f2clib
      COMMAND make blaslib
      COMMAND make -C INSTALL
      COMMAND make -C SRC
      COMMAND cp F2CLIBS/libf2c.a .
    INSTALL_COMMAND ""
    BUILD_IN_SOURCE TRUE
    LOG_BUILD 1
    # Declare the static libs produced by the make build step so the Ninja
    # generator (used by the Linux CI) knows the geoda link can depend on
    # them; Makefiles otherwise tolerate them appearing as a side effect.
    # NOTE: must be BUILD_BYPRODUCTS - the archives are outputs of the
    # build step, and a bare BYPRODUCTS keyword is silently ignored.
    BUILD_BYPRODUCTS
      "${GEODA_CLAPACK_SRC_DIR}/lapack.a"
      "${GEODA_CLAPACK_SRC_DIR}/blas.a"
      "${GEODA_CLAPACK_SRC_DIR}/libf2c.a")

  add_library(clapack INTERFACE)
  add_dependencies(clapack clapack_ext)
  target_link_libraries(clapack INTERFACE
    "${GEODA_CLAPACK_SRC_DIR}/lapack.a"
    "${GEODA_CLAPACK_SRC_DIR}/blas.a"
    "${GEODA_CLAPACK_SRC_DIR}/libf2c.a")
else()
  if(NOT GEODA_CLAPACK_PATH AND APPLE)
    # Convenience default for local macOS development.
    set(_geoda_default_clapack "${CMAKE_SOURCE_DIR}/BuildTools/macosx/temp/CLAPACK-3.2.1")
    if(EXISTS "${_geoda_default_clapack}/lapack.a")
      set(GEODA_CLAPACK_PATH "${_geoda_default_clapack}" CACHE PATH "" FORCE)
    endif()
  endif()

  if(NOT GEODA_CLAPACK_PATH)
    message(FATAL_ERROR
"GEODA_CLAPACK_PATH is not set. GeoDa links against prebuilt CLAPACK static
libs (LAPACK/BLAS/libf2c translated to C). Provide the directory containing
lapack.a, blas.a and libf2c.a (macOS/Linux) or clapack.lib, BLAS.lib and
libf2c.lib (Windows), or pass -DGEODA_USE_CLAPACK_EXTERNAL=ON to build it
from source. The legacy build.sh script built it as:
    make f2clib; make blaslib; make -C INSTALL; make -C SRC; cp F2CLIBS/libf2c.a .
on the official CLAPACK 3.2.1 tarball.")
  endif()

  add_library(clapack INTERFACE)
  if(APPLE)
    # ld64 rescans archives to a fixpoint, so static order is not critical.
    target_link_libraries(clapack INTERFACE
      "${GEODA_CLAPACK_PATH}/lapack.a"
      "${GEODA_CLAPACK_PATH}/blas.a"
      "${GEODA_CLAPACK_PATH}/libf2c.a")
  elseif(WIN32)
    target_link_libraries(clapack INTERFACE
      "${GEODA_CLAPACK_PATH}/BLAS.lib"
      "${GEODA_CLAPACK_PATH}/clapack.lib"
      "${GEODA_CLAPACK_PATH}/libf2c.lib")
  else()
    # GNU ld scans archives once: wrap in a group so lapack/blas/f2c can be
    # resolved in any order.
    target_link_libraries(clapack INTERFACE
      -Wl,--start-group
      "${GEODA_CLAPACK_PATH}/lapack.a"
      "${GEODA_CLAPACK_PATH}/blas.a"
      "${GEODA_CLAPACK_PATH}/libf2c.a"
      -Wl,--end-group)
  endif()
endif()

# ---------------------------------------------------------------------------
# Sources
#
# Whitelist of the directories that belong to the application build. This
# deliberately excludes BuildTools/, tests/, arizona/ (dead) and Generic/
# (legacy duplicates) by construction. libgdiam/gdiam_test.cpp has its own
# main() and is skipped.
# ---------------------------------------------------------------------------
set(GEODA_BUILD_SUBDIRS
  Weights ogl io Algorithms DataViewer DialogTools Explore
  kNN libgdiam Regression ShapeOperations VarCalc rc)

file(GLOB GEODA_SOURCES CONFIGURE_DEPENDS "${CMAKE_SOURCE_DIR}/*.cpp")
foreach(_subdir ${GEODA_BUILD_SUBDIRS})
  file(GLOB_RECURSE _sub_sources CONFIGURE_DEPENDS
    "${CMAKE_SOURCE_DIR}/${_subdir}/*.cpp" "${CMAKE_SOURCE_DIR}/${_subdir}/*.c")
  list(APPEND GEODA_SOURCES ${_sub_sources})
endforeach()
list(FILTER GEODA_SOURCES EXCLUDE REGEX "[\\\\/]gdiam_test\\.cpp$")

add_executable(geoda WIN32 MACOSX_BUNDLE ${GEODA_SOURCES})
if(WIN32)
  target_sources(geoda PRIVATE "${CMAKE_SOURCE_DIR}/GeoDa.rc")
endif()

set_target_properties(geoda PROPERTIES
  OUTPUT_NAME GeoDa
  MACOSX_BUNDLE_GUI_IDENTIFIER "edu.uchicago.spatial.GeoDa"
  MACOSX_BUNDLE_BUNDLE_VERSION "${PROJECT_VERSION}"
  MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}"
  MACOSX_BUNDLE_ICON_FILE GeoDa)

target_include_directories(geoda PRIVATE
  "${CMAKE_SOURCE_DIR}"
  "${SPECTRA_INCLUDE_DIR}"
  "${Boost_INCLUDE_DIRS}"
  "${wxWidgets_INCLUDE_DIRS}"
  "${GDAL_INCLUDE_DIRS}"
  "${EIGEN3_INCLUDE_DIR}"
  "${json_spirit_SOURCE_DIR}")
if(APPLE OR WIN32)
  # OpenCL headers are not on the default search path on Windows; the
  # workflow passes -DOpenCL_INCLUDE_DIR pointing at the SDK headers.
  # (macOS finds them via the framework, but the dir is harmless there.)
  target_include_directories(geoda PRIVATE "${OpenCL_INCLUDE_DIR}")
endif()

target_compile_definitions(geoda PRIVATE
  __PROJ6__
  ${wxWidgets_DEFINITIONS})

if(WIN32)
  target_compile_definitions(geoda PRIVATE
    NOMINMAX _WINDOWS BOOST_BIND_NO_PLACEHOLDERS UNICODE _UNICODE)
endif()

if(APPLE)
  # Algorithms/lapacke.c (legacy CLAPACK wrapper) passes ints where char*
  # is expected; newer clang makes this an error by default.
  target_compile_options(geoda PRIVATE -Wno-int-conversion)
endif()

target_link_libraries(geoda PRIVATE
  ${wxWidgets_LIBRARIES}
  Boost::thread Boost::date_time Boost::chrono Boost::atomic Boost::regex
  GDAL::GDAL
  Eigen3::Eigen
  json_spirit
  OpenGL::GL
  OpenGL::GLU
  clapack
  CURL::libcurl
  ZLIB::ZLIB
  Threads::Threads)

if(APPLE)
  target_link_libraries(geoda PRIVATE
    "${OpenCL_LIBRARIES}"
    iconv)
endif()
if(WIN32)
  target_link_libraries(geoda PRIVATE
    "${OpenCL_LIBRARIES}"
    wsock32 comctl32 winmm rpcrt4)
endif()

# ---------------------------------------------------------------------------
# Runtime data
#
# GeoDa expects, next to the binary / inside the .app bundle: the compiled xrc
# resources, cache.sqlite, geoda_prefs.json, the OpenCL kernels, the language
# files, web_plugins, the GDAL data and the PROJ data. Layout (from
# GenUtils.cpp): macOS -> Contents/Resources/, Windows -> exe dir with gdaldata
# renamed to data/, Linux -> exe dir keeping gdaldata/.
# ---------------------------------------------------------------------------
set(GEODA_RUNTIME_FILES
  "${CMAKE_SOURCE_DIR}/BuildTools/CommonDistFiles/cache.sqlite"
  "${CMAKE_SOURCE_DIR}/BuildTools/CommonDistFiles/geoda_prefs.json"
  "${CMAKE_SOURCE_DIR}/rc/data_viewer_dialogs.xrc"
  "${CMAKE_SOURCE_DIR}/rc/dialogs.xrc"
  "${CMAKE_SOURCE_DIR}/rc/menus.xrc"
  "${CMAKE_SOURCE_DIR}/rc/toolbar.xrc"
  "${CMAKE_SOURCE_DIR}/Algorithms/lisa_kernel.cl"
  "${CMAKE_SOURCE_DIR}/Algorithms/distmat_kernel.cl"
  "${CMAKE_SOURCE_DIR}/Algorithms/localjc_kernel.cl")

if(APPLE)
  set(GEODA_RUNTIME_DEST "$<TARGET_BUNDLE_CONTENT_DIR:geoda>/Resources")
  set(GEODA_GDALDATA_DIR "gdaldata")
else()
  set(GEODA_RUNTIME_DEST "$<TARGET_FILE_DIR:geoda>")
  if(WIN32)
    set(GEODA_GDALDATA_DIR "data")
  else()
    set(GEODA_GDALDATA_DIR "gdaldata")
  endif()
endif()

# Create the destination directory first: `cmake -E copy_if_different` treats a
# non-existent destination as a file path, so without this the first copy would
# create a regular file named after the destination directory.
add_custom_command(TARGET geoda POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${GEODA_RUNTIME_DEST}"
  VERBATIM)

foreach(_f ${GEODA_RUNTIME_FILES})
  add_custom_command(TARGET geoda POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${_f}" "${GEODA_RUNTIME_DEST}"
    VERBATIM)
endforeach()

add_custom_command(TARGET geoda POST_BUILD
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/internationalization/lang" "${GEODA_RUNTIME_DEST}/lang"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/BuildTools/CommonDistFiles/web_plugins" "${GEODA_RUNTIME_DEST}/web_plugins"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/BuildTools/CommonDistFiles/gdaldata" "${GEODA_RUNTIME_DEST}/${GEODA_GDALDATA_DIR}"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory
    "${CMAKE_SOURCE_DIR}/BuildTools/CommonDistFiles/proj" "${GEODA_RUNTIME_DEST}/proj"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "${GEODA_RUNTIME_DEST}/basemap_cache"
  VERBATIM)

if(APPLE)
  add_custom_command(TARGET geoda POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different
      "${CMAKE_SOURCE_DIR}/rc/GeoDa.icns" "${GEODA_RUNTIME_DEST}/GeoDa.icns"
    VERBATIM)
endif()

if(APPLE)
  # Bundle the runtime dylib dependencies into Contents/Frameworks so the app
  # is self-contained and portable. The CMake build links GeoDa against
  # homebrew (or other) dylibs by absolute path, which exist only on the build
  # machine; bundle_dylibs.py copies every non-system dylib (recursively, so
  # transitive deps like GDAL's libarchive/openssl/libtiff chain are included)
  # into the bundle and rewrites the load commands to
  # @executable_path/../Frameworks/. Mirrors the legacy install_name.py step of
  # the old BuildTools toolchain. Idempotent: incremental builds skip the
  # already-bundled refs.
  add_custom_command(TARGET geoda POST_BUILD
    COMMAND "${Python3_EXECUTABLE}"
      "${CMAKE_SOURCE_DIR}/BuildTools/macosx/bundle_dylibs.py"
      "$<TARGET_BUNDLE_DIR:geoda>"
    VERBATIM)
endif()

# ---------------------------------------------------------------------------
# Optional: regenerate rc/GdaAppResources.cpp from the .xrc files.
# ---------------------------------------------------------------------------
if(GEODA_REGENERATE_XRC)
  find_program(WXRC_EXECUTABLE wxrc REQUIRED)
  add_custom_command(
    OUTPUT "${CMAKE_SOURCE_DIR}/rc/GdaAppResources.cpp"
    COMMAND "${WXRC_EXECUTABLE}" -c
      "${CMAKE_SOURCE_DIR}/rc/data_viewer_dialogs.xrc"
      "${CMAKE_SOURCE_DIR}/rc/dialogs.xrc"
      "${CMAKE_SOURCE_DIR}/rc/menus.xrc"
      "${CMAKE_SOURCE_DIR}/rc/toolbar.xrc"
      -o "${CMAKE_SOURCE_DIR}/rc/GdaAppResources.cpp"
    DEPENDS
      "${CMAKE_SOURCE_DIR}/rc/data_viewer_dialogs.xrc"
      "${CMAKE_SOURCE_DIR}/rc/dialogs.xrc"
      "${CMAKE_SOURCE_DIR}/rc/menus.xrc"
      "${CMAKE_SOURCE_DIR}/rc/toolbar.xrc"
    VERBATIM)
endif()

# ---------------------------------------------------------------------------
# Tests (optional)
# ---------------------------------------------------------------------------
if(GEODA_BUILD_TESTS)
  enable_testing()
  if(EXISTS "${CMAKE_SOURCE_DIR}/tests/CMakeLists.txt")
    add_subdirectory(tests)
  else()
    message(WARNING "GEODA_BUILD_TESTS=ON but tests/CMakeLists.txt is missing; skipping.")
  endif()
endif()
