using ConFiGifY

length(ARGS) == 1 || error("expected a configuration path")
cfg = load(only(ARGS))
println(cfg["c"]["myel1"])
println(cfg["myfloat2"])
