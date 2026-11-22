# Built-in functions

These functions are available in `.cfgy` expressions in every language binding.

## Scalar math

Each function takes one or more numbers. One argument returns one value. Multiple
arguments return a flat list in argument order. Results are floats except for
`abs`, which preserves the numeric type of each argument.

| Function | Operation | Example |
| --- | --- | --- |
| `abs(x, ...)` | Absolute value | `abs(-3, -0.5)` → `[3, 0.5]` |
| `ceil(x, ...)` | Round toward positive infinity | `ceil(-1.25)` → `-1.0` |
| `floor(x, ...)` | Round toward negative infinity | `floor(-1.25)` → `-2.0` |
| `sqrt(x, ...)` | Square root of a nonnegative number | `sqrt(9, 16)` → `[3.0, 4.0]` |
| `torad(x, ...)` | Convert degrees to radians | `torad(180)` → π |
| `todeg(x, ...)` | Convert radians to degrees | `todeg(pi / 2)` → `90.0` |
| `sin(x, ...)` | Sine of an angle in radians | `sin(torad(90))` → `1.0` |
| `cos(x, ...)` | Cosine of an angle in radians | `cos(0)` → `1.0` |
| `tan(x, ...)` | Tangent of an angle in radians | `tan(0)` → `0.0` |

## Matrices and rotations

| Function | Arguments and result |
| --- | --- |
| `matrix(rows)` | Creates a matrix from a nonempty list of equal-length, nonempty numeric rows. |
| `transpose(a)` | Swaps rows and columns of a matrix or rectangular list of rows. |
| `Rot.from_rpy(roll, pitch, yaw)` | Takes three angles **in degrees**, or a single list of three angles. Returns the 3 × 3 rotation that applies roll, then pitch, then yaw. |
| `Rot.from_quat(x, y, z, w)` | Takes four components, or a single list in **xyzw** order. Normalizes the nonzero quaternion and returns a 3 × 3 rotation matrix. |

Use `a * b` for element-wise multiplication, `a @ b` for matrix products, and
`rotation @ vector` to apply a rotation.

## Random numbers

| Function | Arguments and result |
| --- | --- |
| `Rand.seed(n)` | Creates an independent stream from an integer seed and resets the default generator to that seed. The setting loads as the integer `n`. |
| `Rand.uniform(low, high)` | Draws a float in `[low, high)`. Requires `low < high`. Use `Rand.uniform()` for `[0, 1)`. |
| `Rand.normal(mean, stddev)` | Draws a float from a normal distribution with positive standard deviation. Use `Rand.normal()` for mean 0 and standard deviation 1. |
| `Rand.int(low, high)` | Draws an integer between the two integer bounds, inclusive. Requires `low <= high`. |

Each draw function also accepts a stream created by `Rand.seed`.
`Rand.uniform[seed](...)`, `Rand.normal[seed](...)`, and `Rand.int[seed](...)`
take the same arguments and return a list of two consecutive draws from that
stream. Calls without brackets return one draw from the default generator.
