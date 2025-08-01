#!/usr/bin/env bash

set -e

CHANNEL="${1:-debug}"
TARGET="${2:-tests}"
BUILD_DIR="build/${CHANNEL,,}"

case "$TARGET" in
  tests)
    echo "Running tests for channel: $CHANNEL"
    ctest --test-dir "$BUILD_DIR" --output-on-failure
    ;;
  benchmarks)
    echo "Running benchmarks for channel: $CHANNEL"
    "$BUILD_DIR/oxidize_benchmarks"
    ;;
  *)
    echo "Unknown target: $TARGET"
    echo "Usage: ./run [channel] [tests|benchmarks]"
    exit 1
    ;;
esac
