# Examples

Each example loads the [motion recorder configuration](example.cfgy) from a path supplied at runtime. It prints the sample count (`800`), then the interval in milliseconds (`2.5`).

| Example source | Read the sample count |
| --- | --- |
| [C](c/example.c) | `cfgy_read(cfg, &samples, "recording", "samples")` |
| [C++](cpp/example.cpp) | `cfg["recording"]["samples"]` |
| [C#](csharp/Program.cs) | `cfg["recording"]["samples"]` |
| [Go](go/main.go) | `cfgy.Lookup(cfg, "recording", "samples")` |
| [Java](java/Example.java) | `Cfgy.lookup(cfg, "recording", "samples")` |
| [Julia](julia/example.jl) | `cfg["recording"]["samples"]` |
| [Lua](lua/example.lua) | `cfg["recording"]["samples"]` |
| [MATLAB/Octave](matlab/example_cfgy.m) | `cfg{'recording'}{'samples'}` |
| [Python](python/example.py) | `cfg["recording"]["samples"]` |
| [Rust](rust/example.rs) | `cfg["recording"]["samples"]` |

## Python 3.9+

Run these commands from the repository root with a C++17 compiler and Python development headers installed.

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -e .
python3 examples/python/example.py examples/example.cfgy
```

## Other languages

Build the shared library once from the repository root, then run the commands for your language in the same shell. The commands below use Linux. For other platforms, see the [binding guide](../BINDINGS.md#build-the-shared-library).

```sh
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
cmake --build build
export LD_LIBRARY_PATH="$PWD/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
```

### C and C++

```sh
./build/example-c examples/example.cfgy
./build/example examples/example.cfgy
```

### C# (.NET 8)

```sh
dotnet run --project examples/csharp -- examples/example.cfgy
```

### Go

```sh
export CGO_LDFLAGS="-L$PWD/build"
(cd examples/go && go run . ../example.cfgy)
```

### Java

Set `JAVA_HOME` to a JDK 11 or later with JNI headers.

```sh
cmake -S . -B build -DCFGY_JAVA=ON
cmake --build build --target cfgy_java cfgy_jni
java -Djava.library.path=build -cp build/cfgy.jar examples/java/Example.java examples/example.cfgy
```

### Julia

```sh
julia --project=src/julia examples/julia/example.jl examples/example.cfgy
```

### Lua

Use Lua 5.3 or later with matching development headers and 64-bit integers.

```sh
cmake -S . -B build -DCFGY_LUA=ON
cmake --build build --target cfgy_lua
LUA_CPATH="$PWD/build/lua/?.so;;" lua examples/lua/example.lua examples/example.cfgy
```

### MATLAB

Set `Matlab_ROOT_DIR` to your MATLAB installation when configuring CMake.

```sh
cmake -S . -B build -DCFGY_MATLAB=ON -DMatlab_ROOT_DIR=/path/to/MATLAB
cmake --build build --target cfgy_native
matlab -batch "addpath('src/matlab','build','examples/matlab'); example_cfgy('examples/example.cfgy')"
```

### GNU Octave

Use Octave's development tools to build the MEX module for the same example.

```sh
mkoctfile --mex -Isrc/c src/matlab/native.c -Lbuild -lcfgy -o build/cfgy_native.mex
octave --quiet --eval "addpath('src/matlab','build','examples/matlab'); example_cfgy('examples/example.cfgy')"
```

### Rust

```sh
export CFGY_LIB_DIR="$PWD/build"
cargo run --manifest-path examples/rust/Cargo.toml -- examples/example.cfgy
```
