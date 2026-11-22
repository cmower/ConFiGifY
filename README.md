# ConFiGifY

ConFiGifY lets you define a setting once and reuse it in expressions inside a
YAML-style `.cfgy` file. For example, a sensor's sample rate can determine both
the time between readings and the number of samples to collect.

```yaml
sensor:
  rate_hz: 200

recording:
  duration_s: 2
  interval_ms: 1000 / sensor.rate_hz     # 5.0
  samples: sensor.rate_hz * duration_s   # 400
```

Change `rate_hz` to `100` and the next load gives you a 10 ms interval and 200 samples.

## Supported languages

Use the same file in **C, C++, C#, Go, Java, Julia, Lua, MATLAB, Python, and Rust**.
All ten use the same evaluator, so a Python analysis and a C++ application get
the same values. [Setup and examples by language](BINDINGS.md)

## Python 3.9+

The Python and C++ quick starts below read the bundled [example.cfgy](examples/example.cfgy).

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
