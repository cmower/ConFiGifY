"""Python access to the ConFiGifY C++ evaluator."""

import _cfgy


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


def loads(text, source="<string>"):
    """Evaluate a configuration string and return its root mapping."""
    try:
        return _cfgy.parse(text, str(source), Config)
    except ValueError as error:
        raise ConfigError(str(error)) from None


def load(path):
    """Load a UTF-8 file from a string or path-like object."""
    with open(path, encoding="utf-8", newline="\n") as stream:
        return loads(stream.read(), source=str(path))
