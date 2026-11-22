# ConFiGifY

YAML-style configuration with arithmetic, references, and math functions.
One C++ evaluator powers the [bindings for Python, C, Rust, Go, Java, Julia, C#, Lua, and MATLAB](BINDINGS.md).

## Python 3.9+

```python
import sys
import cfgy

cfg = cfgy.load(sys.argv[1])
print(cfg["c"]["myel1"])  # 12
print(cfg["myfloat2"])    # 9.0
```

Install from the project root with a C++17 compiler and Python development headers.

```sh
python3 -m venv .venv
. .venv/bin/activate
python3 -m pip install -e .
python3 examples/python/example.py examples/example.cfgy
```

## C++17

```cpp
#include <cfgy>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) return 2;
    const auto cfg = cfgy::load(argv[1]);
    const double gain = cfg["myfloat2"];
    std::cout << cfg["c"]["myel1"] << '\n';  // 12
    std::cout << gain << '\n';               // 9
}
```

```sh
cmake -S . -B build
cmake --build build
./build/example examples/example.cfgy
```
