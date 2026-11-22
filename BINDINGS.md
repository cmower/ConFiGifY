# Language bindings

Every binding calls the C++17 evaluator. The [examples](examples) read the same two
values from the path supplied on the command line. Run the commands below from
the repository root.

## Build the shared library

```sh
cmake -S . -B build -DBUILD_SHARED_LIBS=ON
cmake --build build
export LD_LIBRARY_PATH="$PWD/build${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"  # Linux
```

On macOS use `DYLD_LIBRARY_PATH`; on Windows put the directory containing
`cfgy.dll` on `PATH`. To install the C/C++ headers and library under a prefix,
run `cmake --install build --prefix /your/prefix`.

## Python

Pip builds the C++ evaluator into a CPython extension. A source install requires
a C++17 compiler and Python development headers. A wheel for your Python version
and platform includes the evaluator.

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install .
python3 examples/python/example.py examples/example.cfgy
```

`cfgy.load(path)` and `cfgy.loads(text, source="<string>")` return a `Config`.
Use string keys for maps and integer indices for lists. Existing attribute lookup
also works. Syntax and evaluation errors raise `cfgy.ConfigError`.

## C and C++

In C, load the file with `cfgy_load(path)` and check `cfgy_error(cfg)`.
C11's `cfgy_read` takes one literal map key per level. The output variable
selects the type (`int64_t`, `double`, `bool`, or `cfgy_string`).

```c
int64_t samples;
if (cfgy_read(cfg, &samples, "recording", "samples")) {
    printf("%" PRId64 "\n", samples);
}
```

A read returns 1 on success. A missing key or incompatible type returns 0 and
leaves the output untouched. A `double` accepts either an integer or a float.
For lists and maps, use `cfgy_root`, `cfgy_get`, and the node's children.
Strings and nodes belong to the document. Call `cfgy_free(cfg)` when finished,
including on error. For in-memory input, use `cfgy_parse(bytes, length, source)`.

```sh
./build/example-c examples/example.cfgy
./build/example examples/example.cfgy
```

C++ uses `cfgy::load(path)` and `cfgy::loads(text, source)` from `<cfgy>`.

## Rust

Add `cfgy = { path = "/path/to/ConFiGifY/src/rust" }` to your dependencies.
`cfgy::load(path)` and `cfgy::loads(text)` return `Result<Value, String>`.
Index maps and lists with brackets. Use `println!("{}", value)` to print a value
directly, or match its `Value` variant to work with the underlying Rust type.

```sh
export CFGY_LIB_DIR="$PWD/build"
cargo run --manifest-path examples/rust/Cargo.toml -- examples/example.cfgy
```

## Go

Import `github.com/cmower/ConFiGifY/src/go` as `cfgy`. Use brackets for a
top-level key. For nested keys, pass one key per level to
`cfgy.Lookup`. Both forms support Go's usual presence check.

```go
samples, hasSamples := cfgy.Lookup(cfg, "recording", "samples")
intervalMS, hasInterval := cfgy.Lookup(cfg, "recording", "interval_ms")
```

`cfgy.Load(path)` and `cfgy.Loads(text)` return `(map[string]any, error)`.
Integers are `int64` and lists are `[]any`.

```sh
export CGO_LDFLAGS="-L$PWD/build"
(cd examples/go && go run . ../example.cfgy)
```

## Java

Use a JDK with JNI headers and set `JAVA_HOME` to the JDK directory.
Pass one literal key per level to `Cfgy.lookup`. Use `cfg.get` for a top-level key.

```java
var cfg = Cfgy.load(Path.of(args[0]));
System.out.println(Cfgy.lookup(cfg, "recording", "samples"));
System.out.println(Cfgy.lookup(cfg, "recording", "interval_ms"));
```

`io.configify.Cfgy.load(Path)` and `Cfgy.loads(String)` return `Map<String, Object>`.
Integers are `Long` and floats are `Double`. A missing lookup path throws
`NoSuchElementException`. An explicit `null` returns `null`. Parse errors throw
`IllegalArgumentException`.

```sh
cmake -S . -B build -DCFGY_JAVA=ON
cmake --build build --target cfgy_java cfgy_jni
java -Djava.library.path=build -cp build/cfgy.jar examples/java/Example.java examples/example.cfgy
```

The single-file example command requires Java 11 or later.

## Julia

`ConFiGifY.load(path)` and `ConFiGifY.loads(text)` return a
`Dict{String,Any}`. Lists are one-based vectors and `null` becomes `nothing`.
Use the local project or add `src/julia` to your Julia environment with
`Pkg.develop(path="src/julia")`.

```sh
julia --project=src/julia examples/julia/example.jl examples/example.cfgy
```

## C#

Reference `src/csharp/Cfgy.csproj` from a .NET 8 project. Declare the loaded
configuration as `dynamic` to chain map keys and list indices at runtime.

```csharp
dynamic cfg = Cfgy.Config.Load(path);
long samples = cfg["recording"]["samples"];
double intervalMs = cfg["recording"]["interval_ms"];
```

`Load(path)` and `Loads(text)` return a `Dictionary<string, object?>` with
`object?[]` lists. Parse errors throw `FormatException`.

```sh
dotnet run --project examples/csharp -- examples/example.cfgy
```

## Lua

Build against the same Lua installation you use to run scripts (5.3+ with
64-bit integers). `require("cfgy").load(path)` and `cfgy.loads(text)` return
tables. Lists start at one. `cfgy.null` represents null and preserves its place
in lists and maps. The table metatable's `__name` is `cfgy.map` or `cfgy.list`.

```sh
cmake -S . -B build -DCFGY_LUA=ON
cmake --build build --target cfgy_lua
LUA_CPATH="$PWD/build/lua/?.so;;" lua examples/lua/example.lua examples/example.cfgy
```

Use the module's platform extension when setting `LUA_CPATH`.

## MATLAB

Set `Matlab_ROOT_DIR` to your MATLAB installation. CMake builds a MEX file for
that installation. Add both the package directory and the MEX output directory
to the MATLAB path.

```sh
cmake -S . -B build -DCFGY_MATLAB=ON -DMatlab_ROOT_DIR=/path/to/MATLAB
cmake --build build --target cfgy_native
matlab -batch "addpath('src/matlab','build','examples/matlab'); example_cfgy('examples/example.cfgy')"
```

Use one pair of braces per map key or one-based list index.

```matlab
cfg = cfgy.load(path);
disp(cfg{'recording'}{'samples'});       % 800
disp(cfg{'recording'}{'interval_ms'});   % 2.5
disp(cfg{'sensor'}{'channels'}{1});      % ax
```

`cfgy.load(path)` and `cfgy.loads(text)` return a `cfgy.Config`, a
`containers.Map` subclass. Standard map lookups such as `cfg('sensor')`
also work. Lists are cell arrays, integers are `int64`, strings are character
vectors, and `null` becomes `[]`. Parse errors use the identifier `cfgy:Parse`.

## Conformance checks

GitHub Actions runs each binding's tests on Linux, macOS, and Windows for
pull requests targeting `master` and for each push to `master`. The
[status table](README.md#supported-languages) shows the push results. To test
a branch manually, open **Actions**, choose a language and OS, then select
**Run workflow** and the branch.

The fixtures in `tests/` check numeric types, Unicode, nested keys, mixed
lists, nulls, and math errors against the same core. With the relevant bindings
built and the environment variables above set, run their checks from the root.

```sh
cmake -S . -B build -DBUILD_TESTING=ON
cmake --build build
(cd build && ctest --output-on-failure)
python3 tests/python/test_configify.py
cargo test --manifest-path src/rust/Cargo.toml
(cd src/go && go test ./...)
java -Djava.library.path=build -cp build/cfgy.jar tests/java/Check.java tests/conformance.cfgy
julia --project=src/julia tests/julia/check.jl tests/conformance.cfgy
dotnet run --project tests/csharp -- tests/conformance.cfgy
LUA_CPATH="$PWD/build/lua/?.so;;" lua tests/lua/check.lua tests/conformance.cfgy
matlab -batch "addpath('src/matlab','build','tests/matlab'); check_cfgy('tests/conformance.cfgy')"
```
