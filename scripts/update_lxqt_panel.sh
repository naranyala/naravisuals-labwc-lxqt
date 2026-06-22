#!/bin/bash
set -e

# Make sure we are in the workspace directory
cd /media/naranyala/Data/projects-remote/naravisuals-labwc-lxqt

# Ensure a src directory exists for our local builds
mkdir -p src
cd src

if [ ! -d "lxqt-panel" ]; then
    echo "Cloning lxqt-panel..."
    git clone https://github.com/lxqt/lxqt-panel.git
fi

cd lxqt-panel

echo "Previous version/commit:"
git log -1 --format="%h - %s (%ci)" || true

# Fetch the latest changes from the master branch
git checkout master
git pull origin master

echo "New version/commit:"
git log -1 --format="%h - %s (%ci)"

# Export the custom Qt6 paths used by your project
export PATH="/usr/local/qt6-lightweight/bin:$PATH"
export CMAKE_PREFIX_PATH="/usr/local/qt6-lightweight:$CMAKE_PREFIX_PATH"

# Configure, build, and install the panel
mkdir -p build
cd build
cmake -GNinja ..
ninja
sudo ninja install

# Restart the panel to apply changes
killall lxqt-panel || true
