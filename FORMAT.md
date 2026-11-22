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
primary    = number | string | name { "." name } | name "(" [arguments] ")"
           | "[" [arguments] "]" | "(" expression ")"
arguments  = expression { "," expression }
```

Binary operators associate to the left. Decimal integers are signed 64-bit.
A decimal point or exponent produces a float, including in `.5`, `2.`, and
`1e3`. Mixed numeric arithmetic and division produce floats. Two lists can be
concatenated with `+`. Every math function takes one or more numeric arguments
and returns one value for one argument or a list for several. `abs` preserves
each argument's numeric type. `sin`, `cos`, and `tan` use radians. These
functions and `torad`, `todeg`, `sqrt`, `floor`, and `ceil` return floats.
The square root of a negative number is a domain error. Division by zero,
numeric overflow, unknown names, and incompatible operands are errors.

## Reading values

Python and C++ use the same bracket syntax after loading a file.

| Lookup | Value in `examples/example.cfgy` |
| --- | --- |
| `cfg["recording"]["samples"]` | `800` |
| `cfg["recording"]["interval_ms"]` | `2.5` |
| `cfg["sensor"]["channels"][0]` | `"ax"` |

Each string selects one literal key. For example, `cfg["a.b"]` reads the key
named `a.b`, and `cfg["a"]["b"]` reads `b` inside `a`. List indices start at zero.
