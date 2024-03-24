# Copyright (c) 2024 pongasoft
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may not
# use this file except in compliance with the License. You may obtain a copy of
# the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# @author Yan Pujante

import os
from typing import Union, Dict

TAG = '1.90.4'

DISTRIBUTIONS = {
    'master': {
        'hash': '45d128b05af579d58762d992280c373130d817545b4b63c8afc43575b18f45e8e989cf988aeca50df2536a110f2427b29c07b02264a2521f1b1731b2c916f16d'
    },
    'docking': {
        'hash': 'ebb3d0d3e3b80368c56328173e5317fdd4b889474aa728758ae83053b910cc3959181686ec5848db7c83aa21572b3f0de4b5bff5aa55d8f0e107a991fe6572b4'
    }
}

# contrib port information (required)
URL = 'https://github.com/ocornut/imgui'
DESCRIPTION = f'Dear ImGui ({TAG}): Bloat-free Graphical User interface for C++ with minimal dependencies'
LICENSE = 'MIT License'

OPTIONS = {
    'renderer': 'Which renderer to use: opengl3 (default) or wgpu',
    'branch': 'Which branch to use: master (default) or docking',
    'disableDemo': 'A boolean to disable ImGui demo (enabled by default)'
}

# user options (from --use-port)
opts: Dict[str, Union[str, bool]] = {
    'renderer': 'opengl3',
    'branch': 'master',
    'disableDemo': False
}

deps = ['contrib.glfw3']


def get_tag():
    return TAG if opts['branch'] == 'master' else f'{TAG}-{opts["branch"]}'


def get_zip_url():
    # return f'https://github.com/ocornut/imgui/archive/refs/tags/v{get_tag()}.zip'
    return f'file:///Volumes/Vault/Downloads/imgui-{get_tag()}.zip'


def get_lib_name(settings):
    return (f'lib_{name}_{get_tag()}-{opts["renderer"]}' +
            ('-nd' if opts['disableDemo'] else '') +
            '.a')


def get(ports, settings, shared):

    ports.fetch_project(name, get_zip_url(),  sha512hash=DISTRIBUTIONS[opts['branch']]['hash'])

    def create(final):
        root_path = os.path.join(ports.get_dir(), name, f'imgui-{get_tag()}')
        source_path = root_path
        # installing headers to be accessible by code using this port
        ports.install_headers(source_path, target=name)
        ports.install_headers(os.path.join(source_path, 'backends'), target=os.path.join(name, 'backends'))

        srcs = ['imgui.cpp', 'imgui_draw.cpp', 'imgui_tables.cpp', 'imgui_widgets.cpp']
        if not opts['disableDemo']:
            srcs.append('imgui_demo.cpp')
        srcs.append(os.path.join('backends', 'imgui_impl_glfw.cpp'))  # always include glfw backend
        srcs.append(os.path.join('backends', f'imgui_impl_{opts["renderer"]}.cpp'))

        source_include_paths = [os.path.join(source_path, 'backends')]

        # making sure that this port is using the proper GLFW includes (not the one built-in)
        flags = ['--use-port=contrib.glfw3']

        ports.build_port(source_path, final, name, includes=source_include_paths, srcs=srcs, flags=flags)

    lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    if os.path.getmtime(lib) < os.path.getmtime(__file__):
        clear(ports, settings, shared)
        lib = shared.cache.get_lib(get_lib_name(settings), create, what='port')
    return [lib]


def clear(ports, settings, shared):
    shared.cache.erase_lib(get_lib_name(settings))


def process_args(ports):
    args = ['-isystem', ports.get_include_dir(name)]
    if opts['branch'] == 'docking':
        args += ['-DIMGUI_ENABLE_DOCKING=1']
    if opts['disableDemo']:
        args += ['-DIMGUI_DISABLE_DEMO=1']
    return args


def handle_options(options, error_handler):
    for option, value in options.items():
        value = value.lower()
        if option == 'renderer':
            if value in {'opengl3', 'wgpu'}:
                opts[option] = value
            else:
                error_handler(f'[renderer] can be either [opengl3] or [wgpu], got [{value}]')
        elif option == 'branch':
            if value in DISTRIBUTIONS:
                opts[option] = value
            else:
                error_handler(f'[branch] can be either [master] or [docking], got [{value}]')
        if option == 'disableDemo':
            if value in {'true', 'false'}:
                opts[option] = value == 'true'
            else:
                error_handler(f'{option} is expecting a boolean, got {value}')


