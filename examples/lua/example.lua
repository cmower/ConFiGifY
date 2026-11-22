local cfgy = require("cfgy")
local cfg = cfgy.load(assert(arg[1], "expected a configuration path"))
print(cfg["recording"]["samples"])
print(cfg["recording"]["interval_ms"])
