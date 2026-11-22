# Examples

Start in the project root. Pass a relative or absolute configuration path. The [binding guide](../BINDINGS.md) has build and run commands for the other languages.

## Python 3.9+

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -e .
python3 examples/python/example.py examples/example.cfgy
```

## C11 and C++17

```sh
cmake -S . -B build
cmake --build build
./build/example-c examples/example.cfgy
./build/example examples/example.cfgy
```

## Expected output

All three examples print `12` first. Python displays the float as `9.0`, and C and C++ display it as `9`.
