local cfgy = require("cfgy")
local cfg = cfgy.load(assert(arg[1], "expected a configuration path"))
print(cfg["c"]["myel1"])
print(cfg["myfloat2"])
