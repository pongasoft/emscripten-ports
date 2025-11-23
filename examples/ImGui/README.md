### Examples

See [README](../../ports/ImGui/README.md) for details about the port.

There are 3 examples, showing the 3 combinations currently available with ImGui.

### Building

To build the examples, do the following:

#### GLFW + OpenGL3
```sh
# create a build folder
mkdir /tmp/imgui
emcc --shell-file shell.html --use-port=../../ports/ImGui/imgui.py:backend=glfw:renderer=opengl3 main_glfw_opengl3.cpp -o /tmp/imgui/index.html
```

#### GLFW + WebGPU
```sh
# create a build folder
mkdir /tmp/imgui
emcc -s ASYNCIFY=1 --shell-file shell.html --use-port=../../ports/ImGui/imgui.py:backend=glfw:renderer=wgpu main_glfw_wgpu.cpp -o /tmp/imgui/index.html
```

> [!WARNING]
> This example requires the `-s ASYNCIFY=1` option.

#### SDL2 + OpenGL3
```sh
# create a build folder
mkdir /tmp/imgui
emcc --shell-file shell.html --use-port=../../ports/ImGui/imgui.py:backend=sdl2:renderer=opengl3 main_sdl2_opengl3.cpp -o /tmp/imgui/index.html
```

### Running
Each example is built into the `/tmp/imgui` folder. You can then "run" each example with something like this:

```sh
cd /tmp/imgui
python3 -m http.server 8080
```

then point your browser to http://localhost:8080

> [!NOTE]
> At this moment, 
> the WebGPU example can only run in Chrome and Edge, which are the only browsers fully supporting WebGPU.
