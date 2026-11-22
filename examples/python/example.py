import cfgy

cfg = cfgy.load("example.cfgy")
print(cfg.c.myel1)   # 12
print(cfg.myfloat2)  # 9.0
