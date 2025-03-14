### Introduction

![Emscripten Build](https://github.com/pongasoft/emscripten-ports/actions/workflows/main.yml/badge.svg)

Since [Emscripten](https://emscripten.org/) 3.1.54, ports can be external.
This project is meant to collect ports that are not part of Emscripten.
I wrote a blog post about this topic [here](https://www.pongasoft.com/blog/yan/webassembly/2024/02/19/the-power-of-emscripten-ports).

> [!TIP]
> An Emscripten port file is essentially a _recipe_ to fetch the proper artifact(s) and 
> compile the project for the WebAssembly platform.

### Structure

* The `ports` directory contains each port
* The `examples` directory contains examples on how to use each port and should be the primary source for documentation

### Using a port

* Copy the port file in your project
* Use the `--user-port` option to use it

```sh
# get help about a port (use the correct path to the file depending on your project)
> emcc --use-port=ports/ImGui/imgui.py:help
```

### Ports

<table>
  <tr>
    <th>Project</th>
    <th>Version</th>
    <th>Port</th>
    <th>Description</th>
    <th>Examples</th>
  </tr>
  <tr>
    <td><a href="https://github.com/ocornut/imgui">Dear ImGui</a></td>
    <td>1.91.9</td>
    <td><a href="ports/ImGui"><code>imgui.py</code></a></td>
    <td>Bloat-free Graphical User interface for C++ with minimal dependencies</td>
    <td><a href="examples/ImGui">Examples (GLFW + OpenGL3 / GLFW + WebGPU / SDL2 + OpenGL3)</a></td>
  </tr>
</table>


### License

- MIT License. This project can be used according to the terms of the MIT license.
