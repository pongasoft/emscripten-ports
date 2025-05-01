### Introduction

[Dawn](https://dawn.googlesource.com/dawn) is an open-source and cross-platform implementation of the WebGPU standard.
Since 2025/04/28, Dawn [publishes](https://github.com/google/dawn/releases/) an artifact that contains the port.

Following this [thread](https://github.com/emscripten-core/emscripten/issues/23432) on Emscripten, the built-in support
for WebGPU (`-sUSE_WEBGPU=1`) is being deprecated. It is being replaced by Dawn.

At this moment, the released artifact contains information on how to use the port, as well as the port file itself.
This makes it cumbersome to use and does not behave like other Emscripten ports.

The goal of this project is to provide another way to use the port, one that is more in line with other ports (you only 
need the port file).

> [!NOTE]
> This port **requires** a minimum version of Emscripten 4.0.8

### Usage

```sh
# Basic Usage
emcc --use-port=dawn.py ...
```

> [!NOTE]
> At this moment, there is no official release of Dawn, and there are only nightly builds.
> As a result, the version (`tag`) provided in the port will be outdated fairly quickly.
>
> You can either edit the port file manually replacing `TAG` (and `HASH`) with a more recent value or you
> can use the `tag` (and optional `hash`) port option to change to the release you want to use.
> 
> ```sh
> emcc --use-port=dawn.py:tag=v20250428.160623:hash=2f2a2af84bb8d889a6d28e93224f02df9e29a188efd6069a93fa5bf8dac7c44e4259cb17d8ca501016fc8fa94ae81a39c50746f308aeafc0fe5b1e27f77ab104
> ```


> [!NOTE]
> When using CMake, you must use the option in both compile and link phases.
> For example:
> ```cmake
> set(target_options
>    "--use-port=${CMAKE_SOURCE_DIR}/external/emscripten-ports/dawn.py")
> target_compile_options(target PUBLIC "${target_options}")
> target_link_options(target PUBLIC "${target_options}")
> ```

### Options

* `tag`: The tag/version for the version of Dawn you want to use (default to `v20250428.160623`)
* `hash`: The 512 shasum of the artifact downloaded (ok to omit if you do not care)
* `enableCPPBindings`: A boolean to disable CPP bindings (enabled by default)
* `optimizationLevel`: Optimization level: `['0', '1', '2', '3', 'g', 's', 'z']` (default to `2`)
