# GeoDa AppImage Build Workflow

This document explains the AppImage build workflow for GeoDa that creates a portable, self-contained Linux executable with all dependencies embedded.

## Overview

The AppImage build workflow creates a portable version of GeoDa that includes all dependencies (libgdal, wxWidgets, boost, etc.) embedded within the AppImage file. This makes the AppImage larger than the regular .deb packages but ensures portability across different Linux distributions.

## Workflow File

- `.github/workflows/appimage-build.yml` - Main AppImage build workflow

## Key Features

### Embedded Dependencies

The AppImage includes static builds of:
- **libgdal** - Geospatial data abstraction library
- **wxWidgets 3.2.4** - GUI framework (static build)
- **boost 1.75** - C++ libraries (static build) 
- **CLAPACK 3.2.1** - Linear algebra package (static build)
- **JSON Spirit v4.08** - JSON parsing library (static build)
- **Eigen3** - C++ template library for linear algebra
- **Spectra v0.8.0** - C++ library for eigenvalue problems

### Build Environment

- **Base OS**: Ubuntu 22.04 (jammy)
- **Target**: x86_64 architecture
- **Compiler**: GCC with static linking flags

### AppImage Creation

The workflow uses two approaches for AppImage creation:
1. **Primary**: `linuxdeploy` - Automatically bundles dependencies
2. **Fallback**: `appimagetool` - Direct AppImage creation if linuxdeploy fails

## Build Process

### 1. Dependency Creation (`create_appimage_deps.sh`)

This script builds static versions of all dependencies:

```bash
# Static linking configuration
./configure --disable-shared --enable-static --prefix=$GEODA_HOME/libraries
```

Key differences from regular Ubuntu build:
- All libraries built as static (.a files)
- Dependencies installed to local libraries directory
- Fallback URLs for S3-hosted packages

### 2. GeoDa Build (`install_appimage.sh`)

Builds GeoDa with static linking flags:

```bash
export EXTRA_GEODA_LD_FLAGS="-static-libgcc -static-libstdc++ -Wl,--disable-new-dtags"
```

### 3. AppImage Creation

Creates proper AppDir structure:
- `usr/bin/GeoDa` - Main executable
- `usr/lib/` - Bundled libraries
- `usr/share/applications/geoda.desktop` - Desktop file
- `usr/share/icons/` - Application icons
- `AppRun` - Launch script

## Usage

### Triggering the Build

The workflow runs on:
- Push to master branch
- Tags starting with 'v' (e.g., v1.22.0)
- Pull requests to master
- Manual workflow dispatch

### Build Artifacts

The workflow produces:
- `GeoDa-{VERSION}-x86_64.AppImage` - Portable executable

### Running the AppImage

```bash
# Make executable
chmod +x GeoDa-1.22.0-x86_64.AppImage

# Run directly
./GeoDa-1.22.0-x86_64.AppImage

# Or integrate with desktop
./GeoDa-1.22.0-x86_64.AppImage --appimage-extract-and-run
```

## Caching

The workflow uses GitHub Actions caching for:
- Built dependencies in `BuildTools/ubuntu/libraries`
- Downloaded source packages in `BuildTools/ubuntu/temp`

Cache key includes the hash of `create_appimage_deps.sh` to ensure rebuilds when dependencies change.

## File Structure

```
BuildTools/ubuntu/
├── create_appimage_deps.sh    # Builds static dependencies
├── install_appimage.sh        # Builds GeoDa for AppImage
├── package/                   # Desktop files and icons
│   └── usr/share/
│       ├── applications/GeoDa.desktop
│       └── icons/hicolor/*/apps/geoda.png
└── dep/                       # Patches and config files
    ├── CLAPACK-3.2.1/
    └── json_spirit/CMakeLists.txt
```

## Advantages

- **Portability**: Runs on any Linux distribution with basic dependencies
- **Self-contained**: No need to install system packages
- **Version isolation**: Multiple versions can coexist
- **Easy distribution**: Single file download

## Disadvantages

- **Size**: Larger than .deb packages due to embedded libraries
- **Build time**: Longer builds due to static compilation
- **Updates**: Must rebuild entire AppImage for library updates

## Troubleshooting

### Build Failures

1. **Dependency download failures**: Check fallback URLs in `create_appimage_deps.sh`
2. **Static linking errors**: Verify library paths in makefiles
3. **AppImage creation failures**: Check linuxdeploy logs, fallback to appimagetool

### Runtime Issues

1. **Missing libraries**: Check `ldd` output of the GeoDa executable
2. **FUSE errors**: Ensure FUSE is available on target system
3. **Permission errors**: Verify AppImage is executable

## Compatibility

The AppImage should work on:
- Ubuntu 18.04+ 
- Debian 9+
- CentOS 7+
- Fedora 28+
- openSUSE Leap 15+
- Other modern Linux distributions

Minimum requirements:
- glibc 2.27+ (Ubuntu 18.04 level)
- Basic X11 libraries
- FUSE (for AppImage runtime)