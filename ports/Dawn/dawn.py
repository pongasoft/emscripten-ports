# Copyright (c) 2025 pongasoft
#
# Licensed under the MIT License. You may obtain a copy of the License at
#
# https://opensource.org/licenses/MIT
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# @author Yan Pujante

import os
from typing import Union, Dict, Optional

TAG = 'v20250516.124039'
HASH = '994eac4be5f69d8ec83838af9c7b4cc87f15fa22bede589517c169320dd69ab5cf164528f7bd6ec6503b1ef178da3d87df0565d16445dac2a69f98450083dd8f'

# contrib port information (required)
URL = 'https://dawn.googlesource.com/dawn'
DESCRIPTION = 'Dawn is an open-source and cross-platform implementation of the WebGPU standard'
LICENSE = 'Some files: BSD 3-Clause License. Other files: Emscripten\'s license (available under both MIT License and University of Illinois/NCSA Open Source License)'

VALID_OPTION_VALUES = {
  'enableCPPBindings': ['true', 'false'],
  'optimizationLevel': ['0', '1', '2', '3', 'g', 's', 'z']  # all -OX possibilities
}

OPTIONS = {
  'tag': f'The tag/version for the version of Dawn you want to use (default to {TAG})',
  'hash': 'The 512 shasum of the artifact downloaded (ok to omit if you do not care)',
  'enableCPPBindings': 'A boolean to disable CPP bindings (enabled by default)',
  'optimizationLevel': f'Optimization level: {VALID_OPTION_VALUES["optimizationLevel"]} (default to 2)',
}

# user options (from --use-port)
opts: Dict[str, Union[Optional[str], bool]] = {
  'tag': TAG,
  'hash': HASH,
  'enableCPPBindings': True,
  'optimizationLevel': '2'
}

port_name = 'dawn'


def get_zip_url():
  return f'https://github.com/google/dawn/releases/download/{opts["tag"]}/emdawnwebgpu_pkg-{opts["tag"]}.zip'

def get_lib_name(settings):
  return f'lib_{port_name}_{opts["tag"]}-O{opts["optimizationLevel"]}.a'


def get_root_path(ports):
  return os.path.join(ports.get_dir(), port_name, 'emdawnwebgpu_pkg')


def get_include_path(ports):
  return os.path.join(get_root_path(ports), 'webgpu', 'include')


def get_cpp_include_path(ports):
  return os.path.join(get_root_path(ports), 'webgpu_cpp', 'include')


def get_source_path(ports):
  return os.path.join(get_root_path(ports), 'webgpu', 'src')


def get(ports, settings, shared):
  # get the port
  ports.fetch_project(port_name, get_zip_url(), sha512hash=opts['hash'])

  def create(final):
    source_path = get_source_path(ports)
    include_path = get_include_path(ports)

    includes = [include_path]
    srcs = ['webgpu.cpp']
    flags = ['-std=c++17', '-fno-exceptions']
    flags.append(f'-O{opts["optimizationLevel"]}')
    if opts["optimizationLevel"] in ['0', 'g']:
      flags.append('-g')

    ports.build_port(source_path, final, port_name, includes=includes, srcs=srcs, flags=flags)

  lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
  if os.path.getmtime(lib) < os.path.getmtime(__file__):
    clear(ports, settings, shared)
    lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
  return [lib]


def clear(ports, settings, shared):
  shared.cache.erase_lib(get_lib_name(settings))


def linker_setup(ports, settings):
  if settings.USE_WEBGPU:
    raise Exception('dawn may not be used with -sUSE_WEBGPU=1')

  src_dir = get_source_path(ports)

  settings.JS_LIBRARIES += [
    os.path.join(src_dir, 'library_webgpu_enum_tables.js'),
    os.path.join(src_dir, 'library_webgpu_generated_struct_info.js'),
    os.path.join(src_dir, 'library_webgpu_generated_sig_info.js'),
    os.path.join(src_dir, 'library_webgpu.js'),
  ]
  settings.CLOSURE_ARGS += [
    '--externs=' + os.path.join(src_dir, 'webgpu-externs.js'),
  ]

def process_args(ports):
  # It's important that these take precedent over Emscripten's builtin
  # system/include/, which also currently as webgpu headers.
  args = ['-isystem', get_include_path(ports)]
  if opts['enableCPPBindings']:
    args += ['-isystem', get_cpp_include_path(ports)]
  return args


def check_option(option, value, error_handler):
  if option in VALID_OPTION_VALUES and value not in VALID_OPTION_VALUES[option]:
    error_handler(f'[{option}] can be {list(VALID_OPTION_VALUES[option])}, got [{value}]')
  if isinstance(opts[option], bool):
    value = value == 'true'
  return value


def check_required_option(option, value, error_handler):
  if opts[option] is not None and opts[option] != value:
    error_handler(f'[{option}] is already set with incompatible value [{opts[option]}]')
  return check_option(option, value, error_handler)


def handle_options(options, error_handler):
  for option, value in options.items():
    value = value.lower()
    opts[option] = check_option(option, value, error_handler)
  if 'tag' in options and 'hash' not in options:
    opts['hash'] = None
    print(f'Warning: no hash provided for tag {opts["tag"]}. Consider running "curl -sfL {get_zip_url()} | shasum -a 512" and provide the result as an option.')

if __name__ == "__main__":
  print(f'''# To compute checksums run this
curl -sfL https://github.com/google/dawn/releases/download/{TAG}/emdawnwebgpu_pkg-{TAG}.zip | shasum -a 512
''')