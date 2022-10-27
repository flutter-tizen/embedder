# embedder

[![Build](https://github.com/flutter-tizen/embedder/workflows/Build/badge.svg)](https://github.com/flutter-tizen/embedder/actions)

The Flutter embedder for Tizen.

## Build

### Prerequisites

- Linux (x64)
- [depot_tools](https://commondatastorage.googleapis.com/chrome-infra-docs/flat/depot_tools/docs/html/depot_tools_tutorial.html#_setting_up)
- [LLVM](https://apt.llvm.org) (10 or later)
  - `sudo apt install clang-12`
- Additional dependencies
  - `sudo apt install git python3 python3-pip rpm2cpio cpio`
  - `sudo apt install binutils-arm-linux-gnueabi binutils-aarch64-linux-gnu binutils-i686-linux-gnu`
  - `pip3 install requests`

### Environment setup

1. Create an empty directory called `embedder` and `cd` into it.

1. Create a `.gclient` file by running:

   ```sh
   gclient config --name=src --unmanaged https://github.com/flutter-tizen/embedder
   ```

1. Run `gclient sync`.

   ```sh
   cd src
   gclient sync -D --no-history --shallow
   ```

   Note: `gclient sync` must be run every time the `DEPS` file is updated.

### Compile

Note: Replace `/usr/lib/llvm-12` with your own LLVM installation path. For example, you can use `~/tizen-studio/tools/llvm-10` if you have [Tizen Studio](https://developer.tizen.org/development/tizen-studio/download) and the `NativeToolchain-Gcc-9.2` package installed.

- **For arm**

  ```sh
  tools/gn --target-cpu arm --target-toolchain /usr/lib/llvm-12
  ninja -C out/tizen_arm
  ```

- **For arm64**

  ```sh
  tools/gn --target-cpu arm64 --target-toolchain /usr/lib/llvm-12
  ninja -C out/tizen_arm64
  ```

- **For x86**

  ```sh
  tools/gn --target-cpu x86 --target-toolchain /usr/lib/llvm-12
  ninja -C out/tizen_x86
  ```

### Notes

- To build an app (TPK) with the embedder you created in the above, copy the output artifacts (`libflutter_tizen*.so`) into the [flutter-tizen](https://github.com/flutter-tizen/flutter-tizen) tool's cached artifacts directory (`flutter/bin/cache/artifacts/engine`).
- You can specify the output directory name by providing the `--target-dir` option to `tools/gn`.
- The embedder artifacts are statically linked with libc++ (`third_party/libcxx`) by default. To use the target device's `libstdc++.so` instead of the built-in libc++, provide the `--system-cxx` option to `tools/gn`.

## Repository structure

This repository contains the following files and directories.

- `//build`
  - `config`: Contains config files for setting up the build environment for the toolchain.
  - `secondary`: The secondary source tree to find input files for build. See the `.gn` file for details.
- `//flutter`
  - `fml`: A subset of the Flutter engine's [fml](https://github.com/flutter/engine/tree/main/fml) library.
  - `shell/platform/common`: Contains C++ client wrapper and other common code used by embedder implementations. Copied from [flutter/engine](https://github.com/flutter/engine/tree/main/shell/platform/common).
  - `shell/platform/embedder`: Contains `embedder.h` which defines the low-level API for interacting with the Flutter engine. Copied from [flutter/engine](https://github.com/flutter/engine/tree/main/shell/platform/embedder).
  - `shell/platform/tizen`: The Tizen embedder implementation.
    - `channels`: Built-in plugins based on platform channels.
    - `public`: The public API of the embedder.
  - `third_party/accessibility`: A fork of the chromium accessibility library. Copied from [flutter/engine](https://github.com/flutter/engine/tree/main/third_party/accessibility).
- `//tools`
  - `check_symbols.py`: A script to check whether the build artifacts contain symbols not in the allowlist. Used by continuous integration.
  - `download_engine.py`: A script to download engine artifacts required for linking. Executed as a hook by `gclient sync`.
  - `generate_sysroot.py`: A script to generate Tizen sysroots for arm/arm64/x86. Executed as a hook by `gclient sync`.
  - `gn`: A script to automate running `gn gen`.
- `.gn`: The top-level `.gn` file referenced by `gn`.
- `DEPS`: The DEPS file used by `gclient sync` to install dependencies and execute hooks.
