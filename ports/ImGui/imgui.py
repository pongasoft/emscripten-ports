# Copyright (c) 2024 pongasoft
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

TAG = '1.92.1'

# Run this file as a script to see which command to run to generate the checksums
DISTRIBUTIONS = {
    'master': {
        'hash': '233ce0af1b5ad8878fe7216c80950db7f3d276ad0b1a43eb367d4bc48599e03a9ba25f38161b6ccf7051514a126b6571532fc6949de27abe61d64d171770da5c'
    },
    'docking': {
        'hash': 'cb232552484e5da30edf5e56228fc671d65c66188bee06fbce9a12d67ca4b1e3f43bda0230eefe9cb60d2f683c73b53e76690634c8797c076b4d02407ac230a5'
    }
}

# contrib port information (required)
URL = 'https://github.com/ocornut/imgui'
DESCRIPTION = f'Dear ImGui ({TAG}): Bloat-free Graphical User interface for C++ with minimal dependencies'
LICENSE = 'MIT License'

VALID_OPTION_VALUES = {
    'renderer': ['opengl3', 'wgpu'],
    'backend': ['sdl2', 'glfw'],
    'branch': DISTRIBUTIONS.keys(),
    'disableDemo': ['true', 'false'],
    'disableImGuiStdLib': ['true', 'false'],
    'disableDefaultFont': ['true', 'false'],
    'optimizationLevel': ['0', '1', '2', '3', 'g', 's', 'z']  # all -OX possibilities
}

# key is backend, value is set of possible renderers
VALID_RENDERERS = {
    'glfw': {'opengl3', 'wgpu'},
    'sdl2': {'opengl3'}
}

OPTIONS = {
    'renderer': f'Which renderer to use: {VALID_OPTION_VALUES["renderer"]} (required)',
    'backend': f'Which backend to use: {VALID_OPTION_VALUES["backend"]} (required)',
    'branch': 'Which branch to use: master or docking (default to master)',
    'disableDemo': 'A boolean to disable ImGui demo (enabled by default)',
    'disableImGuiStdLib': 'A boolean to disable misc/cpp/imgui_stdlib.cpp (enabled by default)',
    'disableDefaultFont': 'A boolean to disable the default font (enabled by default)',
    'optimizationLevel': f'Optimization level: {VALID_OPTION_VALUES["optimizationLevel"]} (default to 2)',
}

# user options (from --use-port)
opts: Dict[str, Union[Optional[str], bool]] = {
    'renderer': None,
    'backend': None,
    'branch': 'master',
    'disableDemo': False,
    'disableImGuiStdLib': False,
    'disableDefaultFont': False,
    'optimizationLevel': '2'
}

deps = []

port_name = 'imgui'


def get_tag():
    return TAG if opts['branch'] == 'master' else f'{TAG}-{opts["branch"]}'


def get_zip_url():
    return f'https://github.com/ocornut/imgui/archive/refs/tags/v{get_tag()}.zip'


def get_lib_name(settings):
    return (f'lib_{port_name}_{get_tag()}-{opts["backend"]}-{opts["renderer"]}-O{opts["optimizationLevel"]}' +
            ('-nd' if opts['disableDemo'] else '') +
            ('-nl' if opts['disableImGuiStdLib'] else '') +
            ('-nf' if opts['disableDefaultFont'] else '') +
            '.a')


def get(ports, settings, shared):
    from tools import utils

    if opts['backend'] is None or opts['renderer'] is None:
        utils.exit_with_error(f'imgui port requires both backend and renderer options to be defined')

    ports.fetch_project(port_name, get_zip_url(), sha512hash=DISTRIBUTIONS[opts['branch']]['hash'])

    def create(final):
        root_path = os.path.join(ports.get_dir(), port_name, f'imgui-{get_tag()}')
        source_path = root_path

        # this port does not install the headers on purpose (see process_args)
        # a) there is no need (simply refer to the unzipped content)
        # b) avoids any potential issue between docking/master headers being different

        srcs = ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']
        if not opts['disableDemo']:
            srcs.append('imgui_demo.cpp')
        if not opts['disableImGuiStdLib']:
            srcs.append('misc/cpp/imgui_stdlib.cpp')
        srcs.append(os.path.join('backends', f'imgui_impl_{opts["backend"]}.cpp'))
        srcs.append(os.path.join('backends', f'imgui_impl_{opts["renderer"]}.cpp'))

        flags = [f'--use-port={value}' for value in deps]
        flags.append(f'-O{opts["optimizationLevel"]}')
        flags.append('-Wno-nontrivial-memaccess')

        if opts['disableDefaultFont']:
            flags.append('-DIMGUI_DISABLE_DEFAULT_FONT')

        ports.build_port(source_path, final, port_name, srcs=srcs, flags=flags)

    lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    if os.path.getmtime(lib) < os.path.getmtime(__file__):
        clear(ports, settings, shared)
        lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    return [lib]


def clear(ports, settings, shared):
    shared.cache.erase_lib(get_lib_name(settings))


def process_args(ports):
    # makes the imgui files accessible directly (ex: #include <imgui.h>)
    args = ['-I', os.path.join(ports.get_dir(), port_name, f'imgui-{get_tag()}')]
    if opts['branch'] == 'docking':
        args += ['-DIMGUI_ENABLE_DOCKING=1']
    if opts['disableDemo']:
        args += ['-DIMGUI_DISABLE_DEMO=1']
    return args


def linker_setup(ports, settings):
    if opts['renderer'] == 'wgpu':
        settings.USE_WEBGPU = 1
    if opts['backend'] == 'glfw':
        settings.MIN_WEBGL_VERSION = 2
        settings.MAX_WEBGL_VERSION = 2


def check_option(option, value, error_handler):
    if value not in VALID_OPTION_VALUES[option]:
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
        if option == 'renderer' or option == 'backend':
            opts[option] = check_required_option(option, value, error_handler)
        else:
            opts[option] = check_option(option, value, error_handler)

    if opts['backend'] is None or opts['renderer'] is None:
        error_handler(f'both backend and renderer options must be defined')

    if opts['renderer'] not in VALID_RENDERERS[opts['backend']]:
        error_handler(f'backend [{opts["backend"]}] does not support [{opts["renderer"]}] renderer')

    if opts['backend'] == 'glfw':
        glfw3_options = {'optimizationLevel': opts['optimizationLevel']}
        if opts['renderer'] == 'wgpu':
          glfw3_options['disableWebGL2'] = 'true'
        glfw3_options = ':'.join(f"{key}={value}" for key, value in glfw3_options.items())
        deps.append(f"contrib.glfw3:{glfw3_options}")
    else:
        deps.append('sdl2')

if __name__ == "__main__":
    print(f'''# To compute checksums run this
# master    
curl -sfL https://github.com/ocornut/imgui/archive/refs/tags/v{TAG}.zip | shasum -a 512
# docking    
curl -sfL https://github.com/ocornut/imgui/archive/refs/tags/v{TAG}-docking.zip | shasum -a 512
''')