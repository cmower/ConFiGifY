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

All ten languages use the same C++ evaluator. [Browse and run the examples](examples/README.md).

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
