# ConFiGifY file format

ConFiGifY borrows indentation and `key: value` syntax from YAML. The formats
are incompatible. For example, ConFiGifY evaluates `samples: 2 * 400` as the
integer `800`. A YAML parser reads that expression as a string.

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

## References

A setting can refer to a key defined before or after it.

```text
a: 2 * b  # 10
b: 5
```

Lookup starts in the current mapping and continues through its enclosing
mappings. The nearest definition wins, even if it appears later. Use a dotted
path such as `sensor.rate_hz` to read a nested field. The built-in names
`true`, `false`, `null`, and `pi` take precedence when used on their own.

The parser visits settings in file order and evaluates each setting's dependencies
first. It evaluates each setting once, so two references to the same random draw
return the same value. An unknown name or a circular reference is a load error.

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

```text
expression = product { ("+" | "-") product }
product    = unary { ("*" | "/" | "@") unary }
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
concatenated with `+`. Division by zero, numeric overflow, unknown names, and
incompatible operands are errors. The [built-in function reference](doc/functions.md)
lists every function, its arguments, and its return type.

## Matrices and rotations

`matrix(rows)` accepts a nonempty rectangular list of nonempty numeric rows.
Matrix arithmetic uses double precision. Each binding returns the result as a
list of rows, so existing key and list indexing works.

```text
a: matrix([[1, 2], [3, 4]])
b: [[5, 6], [7, 8]]
cells: a * b                 # [[5, 12], [21, 32]]
product: a @ b               # [[19, 22], [43, 50]]
vector: transpose(a) @ [1, 0]  # [1, 2]
```

For two matrices, `*` multiplies corresponding entries and requires equal
shapes. With `@`, an *m × n* matrix times an *n × p* matrix produces an
*m × p* matrix. A flat numeric list on the right of `@` acts as a column
vector and the result is a flat list. `*`, `/`, and `@` have equal precedence
and evaluate left to right.

When updating an older `.cfgy` file, replace `*` with `@` wherever it composes
matrices or applies a matrix to a vector. Keep `2 * a` or `a * 2` for scalar
scaling. Matrices also support unary signs and division by a nonzero scalar.

Plain row lists can be used directly for products or subtraction. Addition
and subtraction use matching shapes. For addition,
wrap at least one operand in `matrix(...)` to select elementwise addition.
Two plain lists joined with `+` always concatenate.

```text
rpy: [90, 0, 180]
rotation: Rot.from_rpy(rpy) @ Rot.from_quat(0, 0, 1, 0)
direction: rotation @ [0, 1, 0]  # approximately [0, 0, -1]
```

Both constructors accept separate components or a single list and return a
3×3 matrix. `Rot.from_rpy(roll, pitch, yaw)` takes angles in degrees and
computes the active, right-handed rotation `Rz(yaw) @ Ry(pitch) @ Rx(roll)`.
For angles already in radians, use `Rot.from_rpy(todeg(roll, pitch, yaw))`.
`Rot.from_quat(x, y, z, w)` normalizes a nonzero quaternion with the scalar
component last. In `A @ B @ v`, rotation `B` acts on the vector first.

## Random numbers

```text
seed1: Rand.seed(42)
seed2: Rand.seed(12)
random_number: Rand.uniform[seed1](-1, 1)  # two floats
noise: Rand.normal[seed2](0, 0.02)
next_pair: Rand.uniform[seed1](-1, 1)
```

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
