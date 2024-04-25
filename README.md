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
  - `sudo apt install git python3 rpm2cpio cpio`
  - `sudo apt install binutils-arm-linux-gnueabi binutils-aarch64-linux-gnu binutils-i686-linux-gnu`

### Environment setup

1. Create an empty directory called `embedder` and `cd` into it.

1. Create a `.gclient` file by running:

   ```sh
   gclient config --name=src --unmanaged https://github.com/flutter-tizen/embedder
   ```

1. Run `gclient sync` to fetch all the source code and dependencies.

   Note: `gclient sync` must be run every time the `DEPS` file (or the `generate_sysroot.py` script) is updated.

### Compile

`cd` into the generated `src` directory and run the following commands.

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

- To build an app (TPK) with the embedder generated in the above, copy the output artifacts (`libflutter_tizen*.so`) into the [flutter-tizen](https://github.com/flutter-tizen/flutter-tizen) tool's cached artifacts directory (`flutter/bin/cache/artifacts/engine`) and run `flutter-tizen run` or `flutter-tizen build tpk`.
- To use the target device's `libstdc++.so` instead of the embedder's built-in libc++ (`third_party/libcxx`), provide the `--system-cxx` option to `tools/gn`.
- Building NUI-related code requires a sysroot for Tizen 6.5 or above and the `--api-version 6.5` option.

## Repository structure

This repository contains the following files and directories.

- `//build`
  - `config`: Contains GN configs for setting up the build environment for the toolchain.
  - `secondary`: The secondary source tree to find input files for build. See the `.gn` file for details.
- `//flutter`
  - `fml`: A subset of the Flutter engine's [fml](https://github.com/flutter/engine/tree/main/fml) library.
  - `shell/platform/common`: Contains C++ client wrapper and other common code used by the embedder implementation. Copied from [flutter/engine](https://github.com/flutter/engine/tree/main/shell/platform/common).
  - `shell/platform/embedder`: Contains `embedder.h` which defines a low-level API for interacting with the Flutter engine. Copied from [flutter/engine](https://github.com/flutter/engine/tree/main/shell/platform/embedder).
  - `shell/platform/tizen`: The Tizen embedder implementation.
    - `channels`: Internal plugins based on platform channels.
    - `public`: The public API of the embedder.
  - `third_party/accessibility`: A fork of the chromium accessibility library. Copied from [flutter/engine](https://github.com/flutter/engine/tree/main/third_party/accessibility).
- `//tools`
  - `download_engine.py`: A script to download engine artifacts required for linking. Executed as a hook by `gclient sync`. The downloaded artifacts can be found in `//engine`.
  - `generate_sysroot.py`: A script to generate Tizen sysroots for arm/arm64/x86. Executed as a hook by `gclient sync`. The sysroots are by default generated in `//sysroot`.
  - `gn`: A script to automate running `gn gen`.
- `.gn`: The top-level `.gn` file referenced by `gn`.
- `DEPS`: The DEPS file used by `gclient sync` to install dependencies and execute hooks.
