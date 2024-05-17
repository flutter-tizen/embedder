#!/usr/bin/env python3
# Copyright 2022 Samsung Electronics Co., Ltd. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

import argparse
import os
import re
import shutil
import subprocess
import sys
import urllib.parse
import urllib.request
from pathlib import Path


base_packages = [
  'gcc',
  'glibc',
  'glibc-devel',
  'libgcc',
  'libstdc++',
  'libstdc++-devel',
  'linux-glibc-devel',
]

unified_packages = [
  'atk-devel',
  'at-spi2-atk-devel',
  'capi-appfw-application',
  'capi-appfw-application-devel',
  'capi-appfw-app-common',
  'capi-appfw-app-common-devel',
  'capi-appfw-app-control',
  'capi-appfw-app-control-devel',
  'capi-base-common',
  'capi-base-common-devel',
  'capi-base-utils',
  'capi-base-utils-devel',
  'capi-system-info',
  'capi-system-info-devel',
  'capi-system-system-settings',
  'capi-system-system-settings-devel',
  'capi-ui-efl-util',
  'capi-ui-efl-util-devel',
  'coregl',
  'coregl-devel',
  'ecore-con-devel',
  'ecore-core',
  'ecore-core-devel',
  'ecore-evas',
  'ecore-evas-devel',
  'ecore-file-devel',
  'ecore-imf',
  'ecore-imf-devel',
  'ecore-imf-evas',
  'ecore-imf-evas-devel',
  'ecore-input',
  'ecore-input-devel',
  'ecore-wayland',
  'ecore-wayland-devel',
  'ecore-wl2',
  'ecore-wl2-devel',
  'edje-devel',
  'eet',
  'eet-devel',
  'efl-devel',
  'efl-extension',
  'efl-extension-devel',
  'efreet-devel',
  'eina',
  'eina-devel',
  'eina-tools',
  'elementary',
  'elementary-devel',
  'emile-devel',
  'eo-devel',
  'ethumb-devel',
  'evas',
  'evas-devel',
  'glib2-devel',
  'jsoncpp',
  'jsoncpp-devel',
  'libatk',
  'libatk-bridge-2_0-0',
  'libeom',
  'libeom-devel',
  'libfeedback',
  'libfeedback-devel',
  'libdlog',
  'libdlog-devel',
  'libglib',
  'libgobject',
  'libtbm',
  'libtbm-devel',
  'libtdm-client',
  'libtdm-client-devel',
  'libtdm-devel',
  'libwayland-client',
  'libwayland-cursor',
  'libwayland-egl',
  'libwayland-extension-client',
  'libxkbcommon',
  'libxkbcommon-devel',
  'tzsh',
  'tzsh-devel',
  'vconf-compat',
  'vconf-compat-devel',
  'vconf-internal-keys-devel',
  'vulkan-headers',
  'vulkan-loader',
  'vulkan-loader-devel',
  'wayland-extension-client-devel',
  'wayland-devel',
]

# Only available for Tizen 6.5 and above.
dali_packages = [
  'dali2',
  'dali2-adaptor',
  'dali2-adaptor-devel',
  'dali2-devel',
  'dali2-toolkit',
  'dali2-toolkit-devel',
]


def generate_sysroot(sysroot: Path, api_version: float, arch: str, quiet=False):
  if arch == 'arm':
    tizen_arch = 'armv7l'
  elif arch == 'arm64':
    tizen_arch = 'aarch64'
  elif arch == 'x86':
    tizen_arch = 'i686'
  else:
    sys.exit('Unknown arch: ' + arch)

  base_repo = 'http://download.tizen.org/snapshots/TIZEN/Tizen-{}/Tizen-{}-Base/latest/repos/standard/packages'.format(
    api_version, api_version)
  unified_repo = 'http://download.tizen.org/snapshots/TIZEN/Tizen-{}/Tizen-{}-Unified/latest/repos/standard/packages'.format(
    api_version, api_version)

  # Retrieve html documents.
  documents = {}
  for url in ['{}/{}'.format(base_repo, tizen_arch),
              '{}/{}'.format(base_repo, 'noarch'),
              '{}/{}'.format(unified_repo, tizen_arch),
              '{}/{}'.format(unified_repo, 'noarch')]:
    request = urllib.request.Request(url)
    with urllib.request.urlopen(request) as response:
      documents[url] = response.read().decode('utf-8')

  # Download packages.
  download_path = sysroot / '.rpms'
  download_path.mkdir(exist_ok=True)
  existing_rpms = [f for f in download_path.iterdir() if f.suffix == '.rpm']

  packages = base_packages + unified_packages
  if api_version >= 6.5:
    packages += dali_packages

  for package in packages:
    quoted = urllib.parse.quote(package)
    pattern = re.escape(quoted) + '-\\d+\\.[\\d_\\.]+-[\\d\\.]+\\..+\\.rpm'

    if any([re.match(pattern, rpm.name) for rpm in existing_rpms]):
      continue

    for parent, doc in documents.items():
      match = re.search('<a href="({})">.+?</a>'.format(pattern), doc)
      if match:
        rpm_url = '{}/{}'.format(parent, match.group(1))
        break

    if match:
      if not quiet:
        print('Downloading ' + rpm_url)
      urllib.request.urlretrieve(rpm_url, download_path / match.group(1))
    else:
      sys.exit('Could not find a package named ' + package)

  # Extract files.
  for rpm in [f for f in download_path.iterdir() if f.suffix == '.rpm']:
    command = 'rpm2cpio {} | cpio -idum --quiet'.format(rpm)
    subprocess.run(command, shell=True, cwd=sysroot, check=True)

  # Create symbolic links.
  asm = sysroot / 'usr' / 'include' / 'asm'
  if not asm.exists():
    os.symlink('asm-' + arch, asm)
  pkgconfig = sysroot / 'usr' / 'lib' / 'pkgconfig'
  if arch == 'arm64' and not pkgconfig.exists():
    os.symlink('../lib64/pkgconfig', pkgconfig)

  # Copy objects required by the linker, such as crtbeginS.o and libgcc.a.
  if arch == 'arm64':
    libpath = sysroot / 'usr' / 'lib64'
  else:
    libpath = sysroot / 'usr' / 'lib'
  subprocess.run('cp gcc/*/*/*.o gcc/*/*/*.a .',
                 shell=True, cwd=libpath, check=True)


def main():
  # Check dependencies.
  for dep in ['rpm2cpio', 'cpio', 'git']:
    if not shutil.which(dep):
      sys.exit('{} is not installed. To install, run:\n'
               '  sudo apt install {}'.format(dep, dep))

  # Parse arguments.
  parser = argparse.ArgumentParser(description='Tizen sysroot generator')
  parser.add_argument('-o', '--out', metavar='PATH', type=str,
                      help='Path to the output directory')
  parser.add_argument('-f', '--force', action='store_true',
                      help='Force re-downloading of packages')
  parser.add_argument('-q', '--quiet', action='store_true',
                      help='Suppress log output')
  parser.add_argument('--api-version', metavar='VER', default=6.0, type=float,
                      help='Target API version (defaults to 6.0)')
  args = parser.parse_args()

  if args.out:
    outpath = Path(args.out)
  else:
    outpath = Path(__file__).parent.parent / 'sysroot'
  outpath.mkdir(exist_ok=True)

  for arch in ['arm', 'arm64', 'x86']:
    sysroot = outpath / arch
    if args.force and sysroot.is_dir():
      shutil.rmtree(sysroot)
    sysroot.mkdir(exist_ok=True)

    if not args.quiet:
      print('Generating sysroot for {}...'.format(arch))
    generate_sysroot(sysroot.resolve(), args.api_version, arch, args.quiet)


# Execute only if run as a script.
if __name__ == '__main__':
  main()
