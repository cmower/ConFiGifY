# ConFiGifY

ConFiGifY lets you write formulas alongside ordinary settings in a YAML-style
`.cfgy` file. Formulas can use earlier settings by name. When your application
loads the file, it gets the calculated values.

This example configures a six-axis motion recorder and simulates a test pulse
along the accelerometer’s Z axis.

```yaml
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
  sensor_to_body: Rot.from_rpy(0, torad(30), 0)

simulation:
  seed: Rand.seed(42)
  accel_gain: matrix([[1.01, 0, 0], [0, 1, 0], [0, 0, 0.99]])
  sensor_pulse_m_s2: [0, 0, Rand.normal(2, 0.02)]
  body_pulse_m_s2: mount.sensor_to_body * accel_gain * sensor_pulse_m_s2
```

Change `rate_hz` to `100` and the same two-second recording has 200 samples,
10 ms apart. The last formula applies the calibration gains to the simulated
pulse, then rotates it into the recorder’s coordinate frame. The [full example](examples/example.cfgy)
also sets a trigger threshold. See the [format guide](FORMAT.md) for all operations.

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
