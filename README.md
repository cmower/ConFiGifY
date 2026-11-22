# ConFiGifY

ConFiGifY lets you define a setting once and reuse it in expressions inside a
YAML-style `.cfgy` file. In this motion recorder, the sample rate sets both the
time between readings and the number of samples to collect.

```yaml
sensor:
  rate_hz: 400

recording:
  before_trigger_s: 1
  after_trigger_s: 1
  duration_s: before_trigger_s + after_trigger_s
  interval_ms: 1000 / sensor.rate_hz     # 2.5
  samples: sensor.rate_hz * duration_s   # 800
```

Change `rate_hz` to `100` and the next load gives you a 10 ms interval and 200 samples.
The full [example.cfgy](examples/example.cfgy) also combines six sensor channels
and calculates the trigger threshold from the sensor's mounting angle.

## Supported languages

All ten languages use the same C++ evaluator. [Setup and examples by language](BINDINGS.md)

| Language | Implemented | Tested on Linux |
| --- | :---: | :---: |
| C | ✓ | ✓ |
| C++ | ✓ | ✓ |
| C# | ✓ | ✓ |
| Go | ✓ | ✓ |
| Java | ✓ | ✓ |
| Julia | ✓ | ✓ |
| Lua | ✓ | ✓ |
| MATLAB/Octave | ✓ | ✓\* |
| Python | ✓ | ✓ |
| Rust | ✓ | ✓ |

\* Tests passed in GNU Octave. Testing in MATLAB is pending.

## Python 3.9+

```python
import sys
import cfgy

cfg = cfgy.load(sys.argv[1])
print(cfg["recording"]["samples"])      # 800
print(cfg["recording"]["interval_ms"])  # 2.5
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
    const double interval_ms = cfg["recording"]["interval_ms"];
    std::cout << cfg["recording"]["samples"] << '\n';  // 800
    std::cout << interval_ms << '\n';                  // 2.5
}
```

```sh
cmake -S . -B build
cmake --build build
./build/example examples/example.cfgy
```
