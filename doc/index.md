# Getting started

ConFiGifY evaluates formulas when your application loads a `.cfgy` file.
Its indentation is inspired by YAML. The two formats are incompatible, so use
the ConFiGifY parser to read `.cfgy` files.

## Write a configuration

In the root of a [ConFiGifY checkout](https://github.com/cmower/ConFiGifY),
save this as `recorder.cfgy`.

```text
sensor:
  rate_hz: 400

recording:
  duration_s: 2
  interval_ms: 1000 / sensor.rate_hz
  samples: sensor.rate_hz * duration_s
```

At 400 Hz the recorder takes 800 samples, 2.5 ms apart. Changing `rate_hz` to
100 gives 200 samples, 10 ms apart.

## Load the file

The Python binding needs Python 3.9+, its development headers, and a C++17
compiler. Install it and run the example in an active virtual environment.

```sh
python3 -m pip install .
python3 examples/python/example.py recorder.cfgy
```

The example reads the path from the command line. These three lines print
`800`, then `2.5`.

```{literalinclude} ../examples/python/example.py
:language: python
:start-at:     cfg = cfgy.load(
:end-at:     print(cfg["recording"]["interval_ms"])
:dedent: 4
```

For C, C++, C#, Go, Java, Julia, Lua, MATLAB, Octave, and Rust, see
[Language bindings](bindings.md).

```{toctree}
:maxdepth: 1
:hidden:

Getting started <self>
File format <format>
Language bindings <bindings>
Worked example <example>
```
