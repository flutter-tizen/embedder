#!/bin/bash
set -e

echo "===== Start svace_builder.sh ====="

EMBEDDER_DIR=$PWD
EMBEDDER_DIR_NAME=$(basename $EMBEDDER_DIR)
PARENT_DIR=$(dirname "$EMBEDDER_DIR")

echo "Changing directory to parent directory: $PARENT_DIR"
cd $PARENT_DIR

# Install depot_tools
if [[ ! -d depot_tools ]]; then
    git clone --depth=1 https://chromium.googlesource.com/chromium/tools/depot_tools.git
fi

# Add depot_tools to PATH 
if [[ ! $PATH == *"depot_tools"* ]]; then
    export PATH="$PARENT_DIR/depot_tools:$PATH"
fi

# Rename the embedder directory to src to minimize code modifications.
if [[ -d "embedder" ]] && [[ $EMBEDDER_DIR_NAME = "embedder" ]]; then
    mv $EMBEDDER_DIR_NAME src
fi

# Run gclient config to set up the .gclient file.
if [[ ! -f .gclient  ]]; then
    gclient config --name=src --unmanaged git@github.com:flutter-tizen/embedder.git
fi

# Retry gclient sync up to 5 times
MAX_RETRIES=5
RETRY_COUNT=0
SYNC_SUCCESS=false
while [[ $RETRY_COUNT -lt $MAX_RETRIES ]]; do
    echo "Attempting gclient sync (attempt $((RETRY_COUNT + 1)) of $MAX_RETRIES)..."
    if gclient sync; then
        echo "gclient sync succeeded!"
        SYNC_SUCCESS=true
        break
    else
        RETRY_COUNT=$((RETRY_COUNT + 1))
        if [ $RETRY_COUNT -lt $MAX_RETRIES ]; then
            echo "gclient sync failed. Retrying in 10 seconds..."
            sleep 10
        else
            echo "gclient sync failed after $MAX_RETRIES attempts."
        fi
    fi
done

# Print the results of gclient sync.
if [[ "$SYNC_SUCCESS" = false ]]; then
    echo "Error: gclient sync failed after $MAX_RETRIES attempts. Exiting."
    exit 1
else
    echo "Success: gclient sync completed successfully."
fi

# Generate sysroot for ARM architecture.
src/tools/generate_sysroot.py --api-version 6.0 --out src/sysroot-6.0

# Run the ninja build.
src/tools/gn \
--target-cpu arm \
--target-toolchain /usr/lib/llvm-17 \
--target-sysroot src/sysroot-6.0/arm \
--target-dir build
ninja -C src/out/build

