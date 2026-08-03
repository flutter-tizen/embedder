# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

deps = {
  'src/third_party/rapidjson': 'https://flutter.googlesource.com/third_party/rapidjson@47253cab97e9cfe99dbd6b90836fc11589d7d802',
  'src/third_party/libcxx': 'https://llvm.googlesource.com/llvm-project/libcxx@bd557f6f764d1e40b62528a13b124ce740624f8f',
  'src/third_party/libcxxabi': 'https://llvm.googlesource.com/llvm-project/libcxxabi@a4dda1589d37a7e4b4f7a81ebad01b1083f2e726',
  'src/third_party/googletest': 'https://github.com/google/googletest@e9907112b47255d50b4d343e7e2160bce8dc85d1',
  'src/third_party/dart': 'https://dart.googlesource.com/sdk.git@d684a576a6aa954ae107a03b2b4e1d61c3bebe93',
  'src/third_party/clang': {
    'packages': [
      {
        'package': 'fuchsia/third_party/clang/linux-amd64',
        'version': 'git_revision:80743bd43fd5b38fedc503308e7a652e23d3ec93'
      }
    ],
    'dep_type': 'cipd',
  },
  'src/third_party/gn': {
    'packages': [
      {
        'package': 'gn/gn/${{platform}}',
        'version': 'git_revision:81b24e01531ecf0eff12ec9359a555ec3944ec4e',
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
