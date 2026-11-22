# Worked example

The motion recorder samples six channels at 400 Hz. Its configuration also
models an accelerometer pulse and converts it to the recorder's coordinate frame.

## Motion recorder configuration

{download}`Download example.cfgy <../examples/example.cfgy>`

```{literalinclude} ../examples/example.cfgy
:language: text
```

`Rand.uniform[noise_seed]` supplies two lateral noise values. Appending `[2]`
gives the three-axis pulse. In the final expression, `accel_gain` calibrates
the pulse and `sensor_to_body` rotates the result by 30 degrees about the Y axis.

## Run an example

Each program takes the configuration path as its argument and prints
`800` samples and a `2.5` ms interval. Choose a language for build and
run instructions, or download its source.

| Instructions | Example source |
| --- | --- |
| [C](bindings.md#c-and-c) | {download}`example.c <../examples/c/example.c>` |
| [C++](bindings.md#c-and-c) | {download}`example.cpp <../examples/cpp/example.cpp>` |
| [C#](bindings.md#c) | {download}`Program.cs <../examples/csharp/Program.cs>` |
| [Go](bindings.md#go) | {download}`main.go <../examples/go/main.go>` |
| [Java](bindings.md#java) | {download}`Example.java <../examples/java/Example.java>` |
| [Julia](bindings.md#julia) | {download}`example.jl <../examples/julia/example.jl>` |
| [Lua](bindings.md#lua) | {download}`example.lua <../examples/lua/example.lua>` |
| [MATLAB](bindings.md#matlab) | {download}`example_cfgy.m <../examples/matlab/example_cfgy.m>` |
| [Octave](bindings.md#gnu-octave) | {download}`example_cfgy.m <../examples/matlab/example_cfgy.m>` |
| [Python](bindings.md#python) | {download}`example.py <../examples/python/example.py>` |
| [Rust](bindings.md#rust) | {download}`example.rs <../examples/rust/example.rs>` |
