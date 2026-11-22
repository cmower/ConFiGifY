# ![ConFiGifY](doc/_static/configify.svg)

ConFiGifY is a configuration file specification for defining settings with
literal values and formulas. Formulas can reference settings defined before or
after them in the same `.cfgy` file. The parser evaluates them when your
application loads the file.

ConFiGifY borrows indentation and key-value syntax from YAML. The two formats
are incompatible.

This example configures a six-axis motion recorder and simulates a test pulse
along the accelerometer’s Z axis.

```text
sensor:
  rate_hz: 400
  accel_channels: ["ax", "ay", "az"]
  gyro_channels: ["gx", "gy", "gz"]
  channels: accel_channels + gyro_channels

recording:
  duration_s: 2
  interval_ms: 1000 / sensor.rate_hz     # 2.5 ms
  samples: sensor.rate_hz * duration_s   # 800 samples

mount:
  sensor_to_body: Rot.from_rpy(0, 30, 0)

simulation:
  noise_seed: Rand.seed(42)
  accel_gain: matrix([[1.01, 0, 0], [0, 1, 0], [0, 0, 0.99]])
  lateral_noise_m_s2: Rand.uniform[noise_seed](-0.02, 0.02)
  sensor_pulse_m_s2: lateral_noise_m_s2 + [2]
  body_pulse_m_s2: mount.sensor_to_body @ accel_gain @ sensor_pulse_m_s2
```

Change `rate_hz` to `100` and the same two-second recording has 200 samples,
10 ms apart. The last formula applies the calibration gains to the simulated
pulse, then rotates it into the recorder’s coordinate frame. The [full example](examples/example.cfgy)
also sets a trigger threshold. The [documentation](https://cmower.github.io/ConFiGifY/)
covers installation, the file format, and language bindings.

## Supported languages

All bindings use the same C++ evaluator. [Browse and run the examples](examples/README.md).

Each badge shows the latest push run on `master` for that language and OS. Select a badge to see the test logs.

| Language | Linux | macOS | Windows |
| --- | :---: | :---: | :---: |
| C | [![C on Linux][c-linux-badge]][c-linux] | [![C on macOS][c-macos-badge]][c-macos] | [![C on Windows][c-windows-badge]][c-windows] |
| C++ | [![C++ on Linux][cpp-linux-badge]][cpp-linux] | [![C++ on macOS][cpp-macos-badge]][cpp-macos] | [![C++ on Windows][cpp-windows-badge]][cpp-windows] |
| C# | [![C# on Linux][csharp-linux-badge]][csharp-linux] | [![C# on macOS][csharp-macos-badge]][csharp-macos] | [![C# on Windows][csharp-windows-badge]][csharp-windows] |
| Go | [![Go on Linux][go-linux-badge]][go-linux] | [![Go on macOS][go-macos-badge]][go-macos] | [![Go on Windows][go-windows-badge]][go-windows] |
| Java | [![Java on Linux][java-linux-badge]][java-linux] | [![Java on macOS][java-macos-badge]][java-macos] | [![Java on Windows][java-windows-badge]][java-windows] |
| Julia | [![Julia on Linux][julia-linux-badge]][julia-linux] | [![Julia on macOS][julia-macos-badge]][julia-macos] | [![Julia on Windows][julia-windows-badge]][julia-windows] |
| Lua | [![Lua on Linux][lua-linux-badge]][lua-linux] | [![Lua on macOS][lua-macos-badge]][lua-macos] | [![Lua on Windows][lua-windows-badge]][lua-windows] |
| MATLAB | Untested | Untested | Untested |
| Octave | [![Octave on Linux][octave-linux-badge]][octave-linux] | [![Octave on macOS][octave-macos-badge]][octave-macos] | [![Octave on Windows][octave-windows-badge]][octave-windows] |
| Python | [![Python on Linux][python-linux-badge]][python-linux] | [![Python on macOS][python-macos-badge]][python-macos] | [![Python on Windows][python-windows-badge]][python-windows] |
| Rust | [![Rust on Linux][rust-linux-badge]][rust-linux] | [![Rust on macOS][rust-macos-badge]][rust-macos] | [![Rust on Windows][rust-windows-badge]][rust-windows] |

[c-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/c-linux.yml/badge.svg?branch=master&event=push
[c-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/c-linux.yml?query=branch%3Amaster+event%3Apush
[c-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/c-macos.yml/badge.svg?branch=master&event=push
[c-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/c-macos.yml?query=branch%3Amaster+event%3Apush
[c-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/c-windows.yml/badge.svg?branch=master&event=push
[c-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/c-windows.yml?query=branch%3Amaster+event%3Apush
[cpp-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/cpp-linux.yml/badge.svg?branch=master&event=push
[cpp-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/cpp-linux.yml?query=branch%3Amaster+event%3Apush
[cpp-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/cpp-macos.yml/badge.svg?branch=master&event=push
[cpp-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/cpp-macos.yml?query=branch%3Amaster+event%3Apush
[cpp-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/cpp-windows.yml/badge.svg?branch=master&event=push
[cpp-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/cpp-windows.yml?query=branch%3Amaster+event%3Apush
[csharp-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/csharp-linux.yml/badge.svg?branch=master&event=push
[csharp-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/csharp-linux.yml?query=branch%3Amaster+event%3Apush
[csharp-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/csharp-macos.yml/badge.svg?branch=master&event=push
[csharp-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/csharp-macos.yml?query=branch%3Amaster+event%3Apush
[csharp-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/csharp-windows.yml/badge.svg?branch=master&event=push
[csharp-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/csharp-windows.yml?query=branch%3Amaster+event%3Apush
[go-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/go-linux.yml/badge.svg?branch=master&event=push
[go-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/go-linux.yml?query=branch%3Amaster+event%3Apush
[go-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/go-macos.yml/badge.svg?branch=master&event=push
[go-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/go-macos.yml?query=branch%3Amaster+event%3Apush
[go-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/go-windows.yml/badge.svg?branch=master&event=push
[go-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/go-windows.yml?query=branch%3Amaster+event%3Apush
[java-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/java-linux.yml/badge.svg?branch=master&event=push
[java-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/java-linux.yml?query=branch%3Amaster+event%3Apush
[java-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/java-macos.yml/badge.svg?branch=master&event=push
[java-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/java-macos.yml?query=branch%3Amaster+event%3Apush
[java-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/java-windows.yml/badge.svg?branch=master&event=push
[java-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/java-windows.yml?query=branch%3Amaster+event%3Apush
[julia-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/julia-linux.yml/badge.svg?branch=master&event=push
[julia-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/julia-linux.yml?query=branch%3Amaster+event%3Apush
[julia-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/julia-macos.yml/badge.svg?branch=master&event=push
[julia-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/julia-macos.yml?query=branch%3Amaster+event%3Apush
[julia-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/julia-windows.yml/badge.svg?branch=master&event=push
[julia-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/julia-windows.yml?query=branch%3Amaster+event%3Apush
[lua-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/lua-linux.yml/badge.svg?branch=master&event=push
[lua-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/lua-linux.yml?query=branch%3Amaster+event%3Apush
[lua-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/lua-macos.yml/badge.svg?branch=master&event=push
[lua-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/lua-macos.yml?query=branch%3Amaster+event%3Apush
[lua-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/lua-windows.yml/badge.svg?branch=master&event=push
[lua-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/lua-windows.yml?query=branch%3Amaster+event%3Apush
[octave-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/octave-linux.yml/badge.svg?branch=master&event=push
[octave-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/octave-linux.yml?query=branch%3Amaster+event%3Apush
[octave-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/octave-macos.yml/badge.svg?branch=master&event=push
[octave-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/octave-macos.yml?query=branch%3Amaster+event%3Apush
[octave-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/octave-windows.yml/badge.svg?branch=master&event=push
[octave-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/octave-windows.yml?query=branch%3Amaster+event%3Apush
[python-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/python-linux.yml/badge.svg?branch=master&event=push
[python-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/python-linux.yml?query=branch%3Amaster+event%3Apush
[python-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/python-macos.yml/badge.svg?branch=master&event=push
[python-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/python-macos.yml?query=branch%3Amaster+event%3Apush
[python-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/python-windows.yml/badge.svg?branch=master&event=push
[python-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/python-windows.yml?query=branch%3Amaster+event%3Apush
[rust-linux-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/rust-linux.yml/badge.svg?branch=master&event=push
[rust-linux]: https://github.com/cmower/ConFiGifY/actions/workflows/rust-linux.yml?query=branch%3Amaster+event%3Apush
[rust-macos-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/rust-macos.yml/badge.svg?branch=master&event=push
[rust-macos]: https://github.com/cmower/ConFiGifY/actions/workflows/rust-macos.yml?query=branch%3Amaster+event%3Apush
[rust-windows-badge]: https://github.com/cmower/ConFiGifY/actions/workflows/rust-windows.yml/badge.svg?branch=master&event=push
[rust-windows]: https://github.com/cmower/ConFiGifY/actions/workflows/rust-windows.yml?query=branch%3Amaster+event%3Apush

## Contributing

To contribute a new feature, [submit a pull request](https://github.com/cmower/ConFiGifY/compare).
To report a problem, [open an issue](https://github.com/cmower/ConFiGifY/issues/new).
