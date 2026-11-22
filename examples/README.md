# Examples

Start in the project root. Pass either example a relative or absolute configuration path.

## Python 3.9+

```sh
python -m pip install .
python examples/python/example.py example.cfgy
```

## C++17

```sh
cmake -S . -B build
cmake --build build
./build/example example.cfgy
```

## Expected output

For `example.cfgy`, both examples print `c.myel1` and `myfloat2` (to one decimal place).

```text
12
9.0
```
