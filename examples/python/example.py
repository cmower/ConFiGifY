import sys

import cfgy

if len(sys.argv) != 2:
    print(f"Usage: {sys.argv[0]} <config-file>", file=sys.stderr)
    sys.exit(2)

try:
    cfg = cfgy.load(sys.argv[1])
    print(cfg.c.myel1)
    print(f"{cfg.myfloat2:.1f}")
except (OSError, ValueError, TypeError, AttributeError) as error:
    sys.exit(str(error))
