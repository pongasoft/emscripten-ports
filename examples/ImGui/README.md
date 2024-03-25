/usr/local/emsdk/emscripten/main/emcc --shell-file shell.html --use-port=../../ports/ImGui/imgui.py:backend=glfw:renderer=opengl3 main_glfw_opengl3.cpp -o /tmp/imgui/index.html
/usr/local/emsdk/emscripten/main/emcc --shell-file shell.html --use-port=../../ports/ImGui/imgui.py:backend=glfw:renderer=wgpu main_glfw_wgpu.cpp -o /tmp/imgui/index.html

/usr/local/emsdk/emscripten/main/emcc --shell-file shell.html --use-port=../../ports/ImGui/imgui.py:backend=sdl2:renderer=opengl3 main_sdl2_opengl3.cpp -o /tmp/imgui/index.html
