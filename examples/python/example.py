import sys

import cfgy

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <config-file>", file=sys.stderr)
    sys.exit(2)

try:
    cfg = cfgy.load(sys.argv[1])
    print(cfg["c"]["myel1"])
    print(cfg["myfloat2"])
except (OSError, ValueError, TypeError, KeyError) as error:
    sys.exit(str(error))
