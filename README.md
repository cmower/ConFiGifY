# ConFiGifY

YAML-style configuration with arithmetic, references, and angle conversions.

## Python 3.9+

```python
import cfgy

cfg = cfgy.load("example.cfgy")
print(cfg.c.myel1)   # 12
print(cfg.myfloat2)  # 9.0
```

Run from the project root.

```sh
PYTHONPATH=src/python python3 examples/python/example.py
```

## C++17

```cpp
#include <cfgy>
#include "example_config.hpp"
#include <iostream>

int main() {
    const auto cfg = cfgy::load<ExampleConfig>("example.cfgy");
    std::cout << cfg.c.myel1 << '\n';  // 12
    double gain = cfg.myfloat2;       // 9.0
}
```

The build generates dot access from the names `c.myel1` and `myfloat2`.
The program reads the configuration at runtime.

```sh
./examples/cpp/build.sh
./build/cfgy-example
```

[Example configuration](example.cfgy) · [File format](FORMAT.md) · [Generate a C++ header](FORMAT.md#generate-a-c-header)
