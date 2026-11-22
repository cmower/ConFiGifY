# ConFiGifY

YAML-style configuration with arithmetic, references, and angle conversions.

## Python 3.9+

```python
import sys
import cfgy

cfg = cfgy.load(sys.argv[1])
print(cfg.c.myel1)   # 12
print(f"{cfg.myfloat2:.1f}")  # 9.0
```

Install from the project root.

```sh
python -m pip install .
python examples/python/example.py example.cfgy
```

## C++17

```cpp
#include <cfgy>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) return 2;
    const auto cfg = cfgy::load(argv[1]);
    std::cout << cfg.at("c.myel1") << '\n';  // 12
    std::cout.precision(1);
    std::cout << std::fixed << cfg["myfloat2"] << '\n';  // 9.0
}
```

```sh
cmake -S . -B build
cmake --build build
./build/example example.cfgy
```

[Run the examples](examples/README.md) · [Example configuration](example.cfgy) · [File format](FORMAT.md)
