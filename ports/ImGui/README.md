### Introduction

Dear ImGui has support for Emscripten (to run in the browser).

### Usage

This port allows using ImGui in Emscripten without needing to check out the project or figuring out what to compile.

```sh
# Basic Usage (BOTH backend and renderer are required option)
emcc --use-port=imgui.py:backend=XXX:renderer=YYY ...
```

> [!NOTE]
> When using CMake, you must use the option in both compile and link phases.
> For example:
> ```cmake
> set(wgpu_shader_toy_options
>    "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/imgui.py:backend=glfw:renderer=wgpu")
> target_compile_options(wgpu_shader_toy PUBLIC "${wgpu_shader_toy_options}")
> target_link_options(wgpu_shader_toy PUBLIC "${wgpu_shader_toy_options}")
> ```

### Options

* `renderer`: Which renderer to use: ['`opengl3`', '`wgpu`'] (required)
* `backend`: Which backend to use: ['`sdl2`', '`glfw`'] (required)
* `branch`: Which branch to use: `master` or `docking` (default to `master`)
* `disableDemo`: A boolean to disable ImGui demo (enabled by default)
* `disableImGuiStdLib`: A boolean to disable `misc/cpp/imgui_stdlib.cpp` (enabled by default)
* `optimizationLevel`: Optimization level: ['0', '1', '2', '3', 'g', 's', 'z'] (default to 2)
