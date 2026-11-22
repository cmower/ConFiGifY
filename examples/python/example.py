from pathlib import Path

import cfgy

cfg = cfgy.load(Path(__file__).resolve().parents[2] / "example.cfgy")
print(cfg.c.myel1)   # 12
print(cfg.myfloat2)  # 9.0
