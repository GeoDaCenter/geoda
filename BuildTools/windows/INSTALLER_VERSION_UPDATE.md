# GeoDa Installer Version Update Solution

## Problem

The GeoDa installer was only using the major and minor version numbers (e.g., "1.22") instead of the full version number including build and subbuild numbers (e.g., "1.22.0.18"). This caused issues when trying to install newer builds over older ones, as Windows would see both as the same version "1.22".

## Solution

A Python script (`update_installer_version.py`) has been created that automatically updates all installer files to use the full version number from `version.h`.

### How it works

1. The script reads the version information from `version.h` in the project root
2. It extracts the major, minor, build, and subbuild version numbers
3. It creates a full version string (e.g., "1.22.0.18")
4. It updates all `.iss` installer files in both 32-bit and 64-bit directories
5. The script is automatically called during the build process

### Files affected

- `BuildTools/windows/installer/32bit/GeoDa.iss`
- `BuildTools/windows/installer/32bit/GeoDa-win7+.iss`
- `BuildTools/windows/installer/64bit/GeoDa.iss`
- `BuildTools/windows/installer/64bit/GeoDa-win7+.iss`

### Integration with build process

The script is automatically called in both `build.bat` and `build_vs2017.bat` before the installer creation step. If the script fails, the build continues with a warning.

### Manual usage

You can also run the script manually:

```bash
cd BuildTools/windows
python update_installer_version.py
```

### Requirements

- Python 3.x
- The script must be run from the `BuildTools/windows` directory
- `version.h` must exist in the project root

### Benefits

- **Automatic version detection**: No need to manually update installer files
- **Consistent versioning**: All installers use the same version from `version.h`
- **Proper upgrade detection**: Windows can now distinguish between different builds
- **Build process integration**: Works seamlessly with existing build scripts

### Example

Before:

```
AppVersion=1.22
```

After:

```
AppVersion=1.22.0.18
```

This allows Windows to properly detect when a newer version (1.22.0.18) is being installed over an older version (1.22.0.12), enabling proper upgrade behavior.
