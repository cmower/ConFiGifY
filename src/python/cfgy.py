"""Read ConFiGifY (.cfgy) files. See FORMAT.md for the language grammar."""

import math
import re
from dataclasses import dataclass


class ConfigError(ValueError):
    """A configuration error with the source name, line and column."""


class Config:
    """A configuration mapping with attribute and key lookup."""

    __slots__ = ("_fields",)

    def __init__(self, fields):
        self._fields = fields

    def __getitem__(self, key):
        return self._fields[key]

    def __getattr__(self, key):
        try:
            return self[key]
        except KeyError:
            raise AttributeError(key) from None

    def __repr__(self):
        return f"Config({self._fields!r})"


@dataclass(frozen=True)
class _Token:
    kind: str
    text: str
    line: int
    column: int


def _fail(source, token, message):
    raise ConfigError(f"{source}:{token.line}:{token.column}: {message}")


_NUMBER = re.compile(r"(?:[0-9]+(?:\.[0-9]*)?|\.[0-9]+)(?:[eE][+-]?[0-9]+)?")
_NAME = re.compile(r"[A-Za-z_][A-Za-z0-9_]*")
_ESCAPES = {"n": "\n", "r": "\r", "t": "\t", "\\": "\\", '"': '"', "'": "'"}


def _lex_line(line, number, source):
    """Keep an end token on each logical line for precise syntax errors."""
    tokens = []
    i = 0
    while i < len(line):
        c = line[i]
        start = i
        if c == "#":
            break
        if c in " \t":
            if c == "\t" and not tokens:
                _fail(source, _Token("", "", number, i + 1), "use spaces for indentation")
            i += 1
            continue
        if c in "\"'":
            i += 1
            chars = []
            while i < len(line) and line[i] != c:
                if line[i] == "\\":
                    i += 1
                    if i == len(line) or line[i] not in _ESCAPES:
                        _fail(source, _Token("", "", number, i), "invalid string escape")
                    chars.append(_ESCAPES[line[i]])
                else:
                    chars.append(line[i])
                i += 1
            if i == len(line):
                _fail(source, _Token("", "", number, start + 1), "unterminated string")
            i += 1
            kind, text = "string", "".join(chars)
        elif match := _NUMBER.match(line, i):
            kind, text, i = "number", match[0], match.end()
        elif match := _NAME.match(line, i):
            kind, text, i = "name", match[0], match.end()
        elif c in ":[],().+-*/":
            kind, text, i = c, c, i + 1
        else:
            _fail(source, _Token("", "", number, i + 1), f"unexpected character {c!r}")
        tokens.append(_Token(kind, text, number, start + 1))
    if tokens:
        tokens.append(_Token("end", "", number, i + 1))
    return tokens


def _lines(text, source):
    for number, line in enumerate(text.split("\n"), 1):
        tokens = _lex_line(line.removesuffix("\r"), number, source)
        if not tokens:
            continue
        # Give "- key: value" the same structure as a dash above an indented map.
        start = 0
        while tokens[start].kind == "-" and tokens[start + 1].kind != "end":
            dash, following = tokens[start:start + 2]
            if following.column == dash.column + 1:
                break
            yield [dash, _Token("end", "", number, dash.column + 1)]
            start += 1
        yield tokens[start:]


_PRECEDENCE = {"+": 1, "-": 1, "*": 2, "/": 2}
_CONSTANTS = {"true": True, "false": False, "null": None, "pi": math.pi}
_MIN_INT, _MAX_INT = -(2**63), 2**63 - 1


class _Expression:
    def __init__(self, tokens, scopes, source):
        self.tokens = tokens
        self.scopes = scopes
        self.source = source
        self.i = 0

    @property
    def current(self):
        return self.tokens[self.i]

    def accept(self, kind):
        if self.current.kind != kind:
            return False
        self.i += 1
        return True

    def require(self, kind):
        if not self.accept(kind):
            _fail(self.source, self.current, f"expected {kind!r}")

    def checked(self, value, token):
        if type(value) is int and not _MIN_INT <= value <= _MAX_INT:
            _fail(self.source, token, "integer out of range")
        if type(value) is float and not math.isfinite(value):
            _fail(self.source, token, "float out of range")
        return value

    def number(self, token, sign=""):
        literal = sign + token.text
        if any(c in literal for c in ".eE"):
            return self.checked(float(literal), token)
        # Strip leading zeroes before conversion, including on very long literals.
        digits = token.text.lstrip("0") or "0"
        if len(digits) > 19:
            _fail(self.source, token, "integer out of range")
        return self.checked(int(sign + digits), token)

    def arguments(self, closing):
        args = []
        if not self.accept(closing):
            while True:
                args.append(self.expression())
                if self.accept(closing):
                    break
                self.require(",")
        return args

    def named(self, token):
        name = token.text
        if self.accept("("):
            if name not in ("torad", "todeg"):
                _fail(self.source, token, f"unknown function {name!r}")
            args = self.arguments(")")
            if not args or any(type(v) not in (int, float) for v in args):
                _fail(self.source, token, f"{name} expects one or more numbers")
            convert = math.radians if name == "torad" else math.degrees
            values = [self.checked(convert(v), token) for v in args]
            return values[0] if len(values) == 1 else values
        if name in _CONSTANTS:
            value = _CONSTANTS[name]
        else:
            for scope in reversed(self.scopes):
                if name in scope:
                    value = scope[name]
                    break
            else:
                _fail(self.source, token, f"unknown reference {name!r}")
        while self.accept("."):
            key = self.current
            self.require("name")
            if not isinstance(value, Config) or key.text not in value._fields:
                _fail(self.source, key, f"unknown field {key.text!r}")
            value = value[key.text]
        return value

    def operand(self):
        token = self.current
        if self.accept("+") or self.accept("-"):
            number = self.current
            if self.accept("number"):
                return self.number(number, token.kind)
            value = self.operand()
            if type(value) not in (int, float):
                _fail(self.source, token, "unary sign expects a number")
            return self.checked(-value if token.kind == "-" else value, token)
        if self.accept("number"):
            return self.number(token)
        if self.accept("string"):
            return token.text
        if self.accept("name"):
            return self.named(token)
        if self.accept("["):
            return self.arguments("]")
        if self.accept("("):
            value = self.expression()
            self.require(")")
            return value
        _fail(self.source, token, "expected a value")

    def apply(self, op, left, right):
        if op.kind == "+" and isinstance(left, list) and isinstance(right, list):
            return left + right
        if type(left) not in (int, float) or type(right) not in (int, float):
            _fail(self.source, op, f"invalid operands for {op.kind!r}")
        if op.kind == "+":
            result = left + right
        elif op.kind == "-":
            result = left - right
        elif op.kind == "*":
            result = left * right
        else:
            if right == 0:
                _fail(self.source, op, "division by zero")
            result = float(left) / float(right)
        return self.checked(result, op)

    def expression(self, minimum=1):
        value = self.operand()
        while (precedence := _PRECEDENCE.get(self.current.kind, 0)) >= minimum:
            op = self.current
            self.i += 1
            value = self.apply(op, value, self.expression(precedence + 1))
        return value

    def parse(self):
        value = self.expression()
        self.require("end")
        return value


class _Document:
    def __init__(self, text, source):
        self.lines = iter(_lines(text, source))
        self.line = next(self.lines, None)
        self.scopes = []
        self.source = source

    def advance(self):
        line = self.line
        self.line = next(self.lines, None)
        return line

    @staticmethod
    def is_entry(line):
        return line[0].kind in ("name", "string") and line[1].kind == ":"

    def value(self, tokens, column):
        if tokens[0].kind != "end":
            return _Expression(tokens, self.scopes, self.source).parse()
        if self.line and self.line[0].column > column:
            return self.block()
        return None

    def block(self):
        column = self.line[0].column
        mapping = self.is_entry(self.line)
        sequence = self.line[0].kind == "-" and self.line[1].kind == "end"
        if not mapping and not sequence:
            return _Expression(self.advance(), self.scopes, self.source).parse()
        if mapping:
            fields = {}
            self.scopes.append(fields)
        else:
            values = []
        while self.line and self.line[0].column >= column:
            line = self.advance()
            key = line[0]
            if key.column != column:
                _fail(self.source, key, "unexpected indentation")
            if mapping:
                if not self.is_entry(line):
                    _fail(self.source, key, "expected a mapping entry")
                if key.text in fields:
                    _fail(self.source, key, f"duplicate key {key.text!r}")
                fields[key.text] = self.value(line[2:], column)
            else:
                if key.kind != "-" or line[1].kind != "end":
                    _fail(self.source, key, "expected a list item")
                values.append(self.value(line[1:], column))
        if mapping:
            self.scopes.pop()
            return Config(fields)
        return values

    def parse(self):
        if self.line is None:
            return Config({})
        if self.line[0].column != 1 or not self.is_entry(self.line):
            _fail(self.source, self.line[0], "expected a top-level mapping")
        return self.block()


def loads(text, source="<string>"):
    """Parse Configify text and return its root Config."""
    return _Document(text, source).parse()


def load(path):
    """Load a UTF-8 Configify file from a string or path-like object."""
    with open(path, encoding="utf-8", newline="\n") as stream:
        return loads(stream.read(), source=str(path))
