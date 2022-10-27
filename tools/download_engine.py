#!/usr/bin/env python3
# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import requests
import shutil
import sys
import urllib.request
from io import BytesIO
from pathlib import Path
from zipfile import ZipFile


# Downloads the latest engine artifacts for use by the linker.
def main():
  engine_dir = Path(__file__).parent.parent / 'engine'
  github_url = 'https://github.com/flutter-tizen/engine/releases'

  stamp = ''
  stamp_file = engine_dir / 'engine.stamp'
  if stamp_file.is_file():
    stamp = stamp_file.read_text().strip()

  version = ''
  # The GitHub REST API cannot be used in the company network due to an
  # "API rate limit exceeded" error. The following is a workaround.
  request = requests.get('{}/latest'.format(github_url))
  redirected_url = request.url
  if '/tag/' in redirected_url:
    version = redirected_url.split('/tag/')[-1]

  if not version:
    sys.exit('Latest tag not found.')
  if version == stamp:
    print('Already downloaded latest version.')
    sys.exit()

  if engine_dir.is_dir():
    shutil.rmtree(engine_dir)
  engine_dir.mkdir()

  names = ['tizen-arm-release.zip',
           'tizen-arm64-release.zip', 'tizen-x86-debug.zip']
  for filename in names:
    arch = filename.split('-')[1]
    print('Downloading libflutter_engine.so for {}...'.format(arch))

    download_path = engine_dir / arch
    download_url = '{}/download/{}/{}'.format(github_url, version, filename)
    with urllib.request.urlopen(download_url) as response:
      with ZipFile(BytesIO(response.read())) as file:
        file.extractall(download_path)

    for file in download_path.iterdir():
      if file.name != 'libflutter_engine.so':
        file.unlink()

  stamp_file.write_text(version)


# Execute only if run as a script.
if __name__ == "__main__":
  main()
