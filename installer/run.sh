#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

# Change to the directory where the script is located
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR"

echo "==> Compiling build system (build.cpp)..."
g++ build.cpp -o build_system

echo "==> Running build system..."
./build_system

echo "==> Launching the application..."
./NaraVisualsThemeManager
