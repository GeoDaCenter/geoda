#!/bin/bash

# Get GDAL version from gdal-config
GDAL_VERSION=$(gdal-config --version 2>/dev/null)

if [ -z "$GDAL_VERSION" ]; then
    echo "Error: gdal-config not found. Make sure GDAL is installed with brew." >&2
    exit 1
fi

# Get GDAL API version (this is what's used in the dylib name)
GDAL_API_VERSION=$(gdal-config --version | cut -d. -f1,2 | tr -d .)

# Extract major version (e.g., "3.7.4" -> "3", "3.11.3" -> "3")
MAJOR_VERSION=$(echo $GDAL_VERSION | cut -d. -f1)
# Extract minor version (e.g., "3.7.4" -> "7", "3.11.3" -> "11")
MINOR_VERSION=$(echo $GDAL_VERSION | cut -d. -f2)
# Extract patch version (e.g., "3.7.4" -> "4", "3.11.3" -> "3")
PATCH_VERSION=$(echo $GDAL_VERSION | cut -d. -f3)

# Use the API version for the dylib name (e.g., "3.8.4" -> "libgdal.38.dylib")
# But first check if the standard symlink exists
if [ -f "/usr/local/opt/gdal/lib/libgdal.dylib" ]; then
    DYLIB_NAME=$(basename $(readlink /usr/local/opt/gdal/lib/libgdal.dylib))
    DYLIB_PATH="/usr/local/opt/gdal/lib/$DYLIB_NAME"
elif [ -f "/opt/homebrew/opt/gdal/lib/libgdal.dylib" ]; then
    DYLIB_NAME=$(basename $(readlink /opt/homebrew/opt/gdal/lib/libgdal.dylib))
    DYLIB_PATH="/opt/homebrew/opt/gdal/lib/$DYLIB_NAME"
else
    echo "Error: Could not find libgdal dylib file" >&2
    exit 1
fi

# Output the results
echo "GDAL_VERSION=$GDAL_VERSION"
echo "MAJOR_VERSION=$MAJOR_VERSION"
echo "DYLIB_NAME=$DYLIB_NAME"
echo "DYLIB_PATH=$DYLIB_PATH" 