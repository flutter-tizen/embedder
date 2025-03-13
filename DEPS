# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

deps = {
  'src/third_party/rapidjson': 'https://fuchsia.googlesource.com/third_party/rapidjson@ef3564c5c8824989393b87df25355baf35ff544b',
  'src/third_party/libcxx': 'https://llvm.googlesource.com/llvm-project/libcxx@44079a4cc04cdeffb9cfe8067bfb3c276fb2bab0',
  'src/third_party/libcxxabi': 'https://llvm.googlesource.com/llvm-project/libcxxabi@2ce528fb5e0f92e57c97ec3ff53b75359d33af12',
  'src/third_party/googletest': 'https://github.com/google/googletest@7f036c5563af7d0329f20e8bb42effb04629f0c0',
  'src/third_party/dart': 'https://dart.googlesource.com/sdk.git@f6ed8d7df6bfdf6fb08b38dd93c2ee1eba476b5a',
  'src/third_party/clang': {
    'packages': [
      {
        'package': 'fuchsia/third_party/clang/linux-amd64',
        'version': 'git_revision:725656bdd885483c39f482a01ea25d67acf39c46'
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
