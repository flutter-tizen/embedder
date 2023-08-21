# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

deps = {
  'src/third_party/rapidjson': 'https://fuchsia.googlesource.com/third_party/rapidjson@ef3564c5c8824989393b87df25355baf35ff544b',
  'src/third_party/libcxx': 'https://llvm.googlesource.com/llvm-project/libcxx@44079a4cc04cdeffb9cfe8067bfb3c276fb2bab0',
  'src/third_party/libcxxabi': 'https://llvm.googlesource.com/llvm-project/libcxxabi@2ce528fb5e0f92e57c97ec3ff53b75359d33af12',
  'src/third_party/googletest': 'https://github.com/google/googletest@054a986a8513149e8374fc669a5fe40117ca6b41',
  'src/third_party/dart': 'https://dart.googlesource.com/sdk.git@1b9c3b35ced9cb3a22f157f827c7ef79faa7b123',
  'src/third_party/clang': {
    'packages': [
      {
        'package': 'fuchsia/third_party/clang/linux-amd64',
        'version': 'git_revision:6d667d4b261e81f325756fdfd5bb43b3b3d2451d'
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
  'src/third_party/ninja': {
    'packages': [
      {
        'package': 'infra/3pp/tools/ninja/${{platform}}',
        'version': 'version:2@1.11.1.chromium.4',
      }
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
