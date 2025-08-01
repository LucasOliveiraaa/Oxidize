#!/usr/bin/env bash

set -e

CHANNEL="${1:-debug}"
BUILD_DIR="build/${CHANNEL,,}"  # lowercase
CMAKE_BUILD_TYPE=""

case "$CHANNEL" in
  debug)
    CMAKE_BUILD_TYPE="Debug"
    ;;
  release)
    CMAKE_BUILD_TYPE="Release"
    ;;
  relwithdebinfo)
    CMAKE_BUILD_TYPE="RelWithDebInfo"
    ;;
  minsizerel)
    CMAKE_BUILD_TYPE="MinSizeRel"
    ;;
  *)
    echo "Unknown build channel: $CHANNEL"
    echo "Usage: ./compile [debug|release|relwithdebinfo|minsizerel]"
    exit 1
    ;;
esac

echo "Building in $BUILD_DIR with type $CMAKE_BUILD_TYPE..."

# Configure
cmake -S . -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE="$CMAKE_BUILD_TYPE"

# Build
cmake --build "$BUILD_DIR"

echo "✅ Build complete!"
