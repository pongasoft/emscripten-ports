### Examples

See [README](../../ports/Dawn/README.md) for details about the port.

### Building

To build the example, do the following:

```sh
# create a build folder
mkdir /tmp/dawn
emcc -sASYNCIFY=1 --use-port=../../ports/Dawn/dawn.py main.cpp -o /tmp/dawn/index.html
```

> [!TIP]
> ```sh
> # when using optimizations, you can add --closure=1 for a smaller code
> emcc -sASYNCIFY=1 --closure=1 -O2 --use-port=../../ports/Dawn/dawn.py main.cpp -o /tmp/dawn/index.html
> ```

### Running
The example is built into the `/tmp/dawn` folder. You can then "run" it with something like this:

```sh
cd /tmp/dawn
python3 -m http.server 8080
```

then point your browser to http://localhost:8080

> [!NOTE]
> At this moment, WebGPU can only run in Chrome and Edge, which are the only browsers fully supporting WebGPU.
