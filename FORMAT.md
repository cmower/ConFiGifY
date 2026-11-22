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
  current mapping and continues through its enclosing mappings. `c.myel1`
  reads a field of an earlier mapping. The built-in names `true`, `false`,
  `null`, and `pi` take precedence when used as unqualified values.

## Expressions

| Expression | Result |
| --- | --- |
| `12 * 2` | Integer `24` |
| `2. + 7.` | Float `9.0` |
| `[1, 2] + mylist1` | `[1, 2, 1, 3]` when `mylist1` is `[1, 3]` |
| `torad(90)` | π/2 as a float |
| `todeg(pi/2)` | `90.0` |
| `torad(90, 0, 180)` | A list of three floats in argument order |

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
concatenated with `+`. The angle functions take one or more numeric arguments
and return a float for one argument or a list for several. Division by zero,
numeric overflow, unknown names, and incompatible operands are errors.

## C++ runtime lookup

`cfgy::load(path)` reads the keys and values when the file is opened.
Use `cfg.at("c.myel1")` or `cfg["c"]["myel1"]` for a nested value.
Bracket lookup also accepts literal keys containing punctuation.

## Optional C++ header generation

For applications that use compile-time member names, the generator can produce
a typed header from the paths the application reads.

```sh
cmake -S . -B build
cmake --build build --target cfgy-gen
./build/cfgy-gen -o settings.hpp --fields Settings c.myel1 myfloat2
```

Include `settings.hpp` and load any file containing those fields.

```cpp
const auto cfg = cfgy::load<Settings>(path);
int count = cfg.c.myel1;
double gain = cfg.myfloat2;
```

If a representative file is available during development, the generator can
infer all fields and their native types in one command.

```sh
./build/cfgy-gen -o settings.hpp sample.cfgy Settings
```

In this mode, lists of matching scalars become typed vectors. Empty, mixed,
or nested lists in the sample become `cfgy::Value::List`. Selected field
names must be usable C++ identifiers. Missing fields and mismatches with an
inferred type raise `cfgy::ConfigError` at load time. Extra keys are accepted.
