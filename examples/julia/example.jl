using ConFiGifY

length(ARGS) == 1 || error("expected a configuration path")
cfg = load(only(ARGS))
println(cfg["recording"]["samples"])
println(cfg["recording"]["interval_ms"])
