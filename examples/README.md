# Examples

Start each section in the project root. Both examples read `example.cfgy`.

## Python 3.9+

```sh
python -m pip install .
cd examples/python
python example.py
```

## C++17

```sh
cmake -S . -B build
cmake --build build
./build/cfgy-example
```

## Expected output

```text
12
9.0
```
