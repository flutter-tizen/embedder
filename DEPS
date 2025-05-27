# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

deps = {
  'src/third_party/rapidjson': 'https://fuchsia.googlesource.com/third_party/rapidjson@ef3564c5c8824989393b87df25355baf35ff544b',
  'src/third_party/libcxx': 'https://llvm.googlesource.com/llvm-project/libcxx@bd557f6f764d1e40b62528a13b124ce740624f8f',
  'src/third_party/libcxxabi': 'https://llvm.googlesource.com/llvm-project/libcxxabi@a4dda1589d37a7e4b4f7a81ebad01b1083f2e726',
  'src/third_party/googletest': 'https://github.com/google/googletest@7f036c5563af7d0329f20e8bb42effb04629f0c0',
  'src/third_party/dart': 'https://dart.googlesource.com/sdk.git@b04011c77cd93e6ab9144af37976733b558d716c',
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
        'version': 'git_revision:7a8aa3a08a13521336853a28c46537ec04338a2d',
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
