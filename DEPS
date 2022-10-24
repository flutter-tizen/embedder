# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

deps = {
  'src/third_party/rapidjson': 'https://fuchsia.googlesource.com/third_party/rapidjson@ef3564c5c8824989393b87df25355baf35ff544b',
  'src/third_party/libcxx': 'https://llvm.googlesource.com/llvm-project/libcxx@54c3dc7343f40254bdb069699202e6d65eda66a2',
  'src/third_party/libcxxabi': 'https://llvm.googlesource.com/llvm-project/libcxxabi@65a68da0f1b102574db316d326a53735b03a4574',
  'src/third_party/icu': 'https://chromium.googlesource.com/chromium/deps/icu.git@12de966fcbe1d1a48dba310aee63807856ffeee8',
  'src/third_party/googletest': 'https://github.com/google/googletest@054a986a8513149e8374fc669a5fe40117ca6b41',
  'src/third_party/dart': 'https://dart.googlesource.com/sdk.git@63c2197b976931c6472d9dc9574f98ff2ae9408c',
  'src/third_party/clang': {
    'packages': [
      {
        'package': 'fuchsia/third_party/clang/linux-amd64',
        'version': 'ugk-KfeqO9fhSfhBFRG4Z-56Kr2AQVSEbku9AEUdotYC'
      }
    ],
    'dep_type': 'cipd',
  },
  'src/third_party/gn': {
    'packages': [
      {
        'package': 'gn/gn/${{platform}}',
        'version': 'git_revision:b79031308cc878488202beb99883ec1f2efd9a6d',
      },
    ],
    'dep_type': 'cipd',
  },
}

hooks = [
  {
    'name': 'Download engine artifacts',
    'pattern': '.',
    'action': ['python3', 'src/tools/download_engine.py'],
  },
  {
    'name': 'Generate Tizen sysroots',
    'pattern': '.',
    'action': ['python3', 'src/tools/generate_sysroot.py', '-q'],
  }
]
