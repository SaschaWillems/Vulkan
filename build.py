# Copyright (c) 2021, Dror Smolarsky
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this
#   list of conditions and the following disclaimer.
#
# * Redistributions in binary form must reproduce the above copyright notice,
# this list of conditions and the following disclaimer in the documentation
# and/or other materials provided with the distribution.
#
# * Neither the name of the copyright holder nor the names of its
# contributors may be used to endorse or promote products derived from
# this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
# AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
# IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
# DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
# FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
# DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
# SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
# CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
# OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
# OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

'''Build Wascha Williems Vulkan samples

This script will
- Download assets
- Update git submodules - assumes that git submodule init was executed already
- Generate build files
- Build the project

The scripts assumes the CMake version is equal or greater than 3.13.

This script only supports desktop builds.
'''

import argparse
import importlib
import os
import platform
import subprocess
import sys

sys.path.insert(0, os.getcwd())
download_assets = importlib.import_module('download_assets')

BUILD_DIR = 'build'


def get_output_dir(build_subdir, architecture, config=''):
    '''Return the output directory for the given architecture and configuration
    '''
    return os.path.join(
        os.getcwd(), BUILD_DIR, build_subdir, architecture, config)


def update_git_submodules():
    '''Update the Git submodules in the repository
    '''
    print('Updating Git submodules')
    update_git_submodule_output = subprocess.run(
        ['git', 'submodule', 'update'])
    if 0 != update_git_submodule_output.returncode:
        raise Exception('Failed to update git submodules')


def gen_build_files(architecture, config):
    '''Use CMake to generate build files
    '''
    print(get_output_dir('cmake_output', architecture, config))
    print(get_output_dir('output', architecture))
    output_dir = get_output_dir('output', architecture, config)
    cmake_gen_output = subprocess.run([
        'cmake',
        '-S', '.',
        '-B', get_output_dir('cmake_output', architecture, config),
        '-DCMAKE_ARCHIVE_OUTPUT_DIRECTORY_{0}={1}'.format(
            config.upper(), os.path.join(output_dir, 'lib')),
        '-DCMAKE_LIBRARY_OUTPUT_DIRECTORY_{0}={1}'.format(
            config.upper(), os.path.join(output_dir, 'bin')),
        '-DCMAKE_RUNTIME_OUTPUT_DIRECTORY_{0}={1}'.format(
            config.upper(), os.path.join(output_dir, 'bin')),
    ])
    if 0 != cmake_gen_output.returncode:
        raise Exception('Failed to generate CMake build files')


def build(architecture, config):
    '''Use CMake to build all the samples
    '''
    build_args = [
        'cmake',
        '--build', '.',
    ]
    if 'windows' == platform.system().lower():
        build_args.extend(['--config', config.capitalize()])
    build_output = subprocess.run(
        build_args,
        cwd=get_output_dir('cmake_output', architecture, config))
    if 0 != build_output.returncode:
        raise Exception('Build failed')


if '__main__' == __name__:
    arg_parser = argparse.ArgumentParser(description=__doc__)
    arg_parser.add_argument(
        '--force-download-assets', dest='force_download_assets',
        action='store_true', default=False,
        help='Force asset download, even if assets were already downloaded')
    arg_parser.add_argument(
        '--skip-submodule-update', dest='skip_submodule_update',
        action='store_true', default=False,
        help='Skip updating the git sub modules')
    arg_parser.add_argument(
        '-a', '--arch', dest='architecture',
        action='store', choices=['x64', 'x86'], default='x64',
        help='Target architecture')
    arg_parser.add_argument(
        '-c', '--config', dest='config',
        action='store', choices=['debug', 'release'], default='release',
        help='Target configuration')
    args = arg_parser.parse_args()
    download_assets.download_assets(args.force_download_assets)
    if not args.skip_submodule_update:
        update_git_submodules()
    gen_build_files(args.architecture, args.config)
    build(args.architecture, args.config)
