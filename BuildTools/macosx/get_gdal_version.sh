#!/bin/bash

# Get GDAL version from gdal-config
GDAL_VERSION=$(gdal-config --version 2>/dev/null)

if [ -z "$GDAL_VERSION" ]; then
    echo "Error: gdal-config not found. Make sure GDAL is installed with brew." >&2
    exit 1
fi

# Extract major version (e.g., "3.7.4" -> "37")
MAJOR_VERSION=$(echo $GDAL_VERSION | cut -d. -f1,2 | tr -d .)

# Always use the standard naming pattern based on major version
DYLIB_NAME="libgdal.${MAJOR_VERSION}.dylib"

# Check if the standard dylib exists
if [ -f "/usr/local/opt/gdal/lib/$DYLIB_NAME" ]; then
    DYLIB_PATH="/usr/local/opt/gdal/lib/$DYLIB_NAME"
elif [ -f "/opt/homebrew/opt/gdal/lib/$DYLIB_NAME" ]; then
    DYLIB_PATH="/opt/homebrew/opt/gdal/lib/$DYLIB_NAME"
else
    # If standard dylib doesn't exist, try to find any libgdal.*.dylib file
    if command -v find >/dev/null 2>&1; then
        FOUND_DYLIB=$(find /usr/local/opt/gdal/lib /opt/homebrew/opt/gdal/lib -name "libgdal.*.dylib" 2>/dev/null | head -1)
        if [ -n "$FOUND_DYLIB" ]; then
            DYLIB_PATH="$FOUND_DYLIB"
            # Still use the standard name for consistency
            DYLIB_NAME="libgdal.${MAJOR_VERSION}.dylib"
        else
            echo "Error: Could not find libgdal dylib file" >&2
            exit 1
        fi
    else
        echo "Error: Could not find libgdal dylib file" >&2
        exit 1
    fi
fi

# Output the results
echo "GDAL_VERSION=$GDAL_VERSION"
echo "MAJOR_VERSION=$MAJOR_VERSION"
echo "DYLIB_NAME=$DYLIB_NAME"
echo "DYLIB_PATH=$DYLIB_PATH" 