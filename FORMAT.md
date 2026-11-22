# ConFiGifY file format

- A UTF-8 document contains one top-level mapping. Each entry begins with
  `key:`. Indent child blocks with spaces and align siblings at the same column.
  An empty document produces an empty mapping. An entry with no value or child
  block has the value `null`.
- Keys are ASCII identifiers (`[A-Za-z_][A-Za-z0-9_]*`) or quoted strings.
  Each key must be unique within its mapping.
- Write a list inline as `[1, 3]`, or start each block item with a dash
  followed by a space or newline. A list item may itself contain a mapping or
  list. For example, `- speed: 12` starts a mapping inside a list.
- Write literal text in single or double quotes. Both styles accept the escapes
  `\\`, `\"`, `\'`, `\n`, `\r`, and `\t`. A `#` outside quotes starts a line
  comment. Each expression fits on one line.
- Bare names in values refer to previously defined keys. Lookup starts in the
  current mapping and continues through its enclosing mappings. Within
  `recording`, the earlier sibling `duration_s` is available by name. Once
  the `sensor` block ends, later entries can use `sensor.rate_hz`.
  The built-in names `true`, `false`,
  `null`, and `pi` take precedence when used as unqualified values.

## Expressions

| Expression | Result |
| --- | --- |
| `12 * 2` | Integer `24` |
| `2. + 7.` | Float `9.0` |
| `["ax", "ay"] + ["az"]` | `["ax", "ay", "az"]` |
| `torad(90)` | π/2 as a float |
| `todeg(pi/2)` | `90.0` |
| `torad(90, 0, 180)` | A list of three floats in argument order |
| `abs(-3, -2.5)` | `[3, 2.5]` |
| `sin(torad(90))` | `1.0` |
| `sqrt(9, 16)` | `[3.0, 4.0]` |

The expression grammar, from lowest precedence to highest, is

```ebnf
expression = product { ("+" | "-") product }
product    = unary { ("*" | "/") unary }
unary      = ("+" | "-") unary | primary
primary    = number | string | name { "." name } | function "(" [arguments] ")"
           | "Rand" "." ("uniform" | "normal" | "int") "[" expression "]" "(" [arguments] ")"
           | "[" [arguments] "]" | "(" expression ")"
function   = name [ "." name ]
arguments  = expression { "," expression }
```

Binary operators associate to the left. Decimal integers are signed 64-bit.
A decimal point or exponent produces a float, including in `.5`, `2.`, and
`1e3`. Mixed numeric arithmetic and division produce floats. Two lists can be
concatenated with `+`. The scalar math functions above take one or more numeric arguments
and returns one value for one argument or a list for several. `abs` preserves
each argument's numeric type. `sin`, `cos`, and `tan` use radians. These
functions and `torad`, `todeg`, `sqrt`, `floor`, and `ceil` return floats.
The square root of a negative number is a domain error. Division by zero,
numeric overflow, unknown names, and incompatible operands are errors.

## Matrices and rotations

`matrix(rows)` accepts a nonempty rectangular list of nonempty numeric rows.
Matrix arithmetic uses double precision. Each binding returns the result as a
list of rows, so existing key and list indexing works.

```yaml
a: matrix([[1, 2], [3, 4]])
b: 2 * a
sum: a + b                 # [[3, 6], [9, 12]]
product: a * b             # [[14, 20], [30, 44]]
vector: transpose(a) * [1, 0]  # [1, 2]
```

Addition and subtraction require equal shapes. For a matrix product, the left
column count must equal the right row count. A flat numeric list on the right
of `*` acts as a column vector. Matrices also support unary signs, multiplication
by a scalar on either side, and division by a nonzero scalar.

Plain row lists can be used directly for products or subtraction. For addition,
wrap at least one operand in `matrix(...)` to select elementwise addition.
Two plain lists joined with `+` always concatenate.

```yaml
rpy: [90, 0, 180]
rotation: Rot.from_rpy(rpy) * Rot.from_quat(0, 0, 1, 0)
direction: rotation * [0, 1, 0]  # approximately [0, 0, -1]
```

Both constructors accept separate components or a single list and return a
3×3 matrix. `Rot.from_rpy(roll, pitch, yaw)` takes angles in degrees and
computes the active, right-handed rotation `Rz(yaw) * Ry(pitch) * Rx(roll)`.
For angles already in radians, use `Rot.from_rpy(todeg(roll, pitch, yaw))`.
`Rot.from_quat(x, y, z, w)` normalizes a nonzero quaternion with the scalar
component last. In `A * B * v`, rotation `B` acts on the vector first.

## Random numbers

```yaml
seed1: Rand.seed(42)
seed2: Rand.seed(12)
random_number: Rand.uniform[seed1](-1, 1)  # two floats
noise: Rand.normal[seed2](0, 0.02)
next_pair: Rand.uniform[seed1](-1, 1)
```

| Call | Result |
| --- | --- |
| `Rand.uniform(low, high)` | Float in `[low, high)`, with `low < high`. Defaults to `[0, 1)`. |
| `Rand.normal(mean, stddev)` | Normal draw with positive standard deviation. Defaults to mean 0 and standard deviation 1. |
| `Rand.int(low, high)` | Integer from `low` through `high`, inclusive. Both bounds must be integers. |
| `Rand.seed(n)` | Creates a stream, resets the default generator to `n`, and exposes the integer seed to your application. |

`Rand.uniform[seed](...)`, `Rand.normal[seed](...)`, and `Rand.int[seed](...)`
each return a list of two consecutive draws from that stream. In the example,
`next_pair` contains the third and fourth uniform draws from `seed1`. A reference
such as `alias: seed1` shares its position. Two calls to `Rand.seed(42)` start
independent streams with the same sequence on the same implementation.

Calls without brackets return one draw from the load's default generator.
The first such draw uses a system seed if the file has not called `Rand.seed`.

## Reading values

Python and C++ use the same bracket syntax after loading a file.

| Lookup | Value in `examples/example.cfgy` |
| --- | --- |
| `cfg["recording"]["samples"]` | `800` |
| `cfg["recording"]["interval_ms"]` | `2.5` |
| `cfg["sensor"]["channels"][0]` | `"ax"` |

Each string selects one literal key. For example, `cfg["a.b"]` reads the key
named `a.b`, and `cfg["a"]["b"]` reads `b` inside `a`. List indices start at zero.
