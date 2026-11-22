# Examples

Start in the project root. Pass either example a relative or absolute configuration path.

## Python 3.9+

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -e .
python3 examples/python/example.py example.cfgy
```

## C++17

```sh
cmake -S . -B build
cmake --build build
./build/example example.cfgy
```

## Expected output

Both examples print `12` first. Python displays the float as `9.0` and C++ displays it as `9`.
