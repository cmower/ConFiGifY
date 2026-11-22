using ConFiGifY, Test
cfg = load(ARGS[1])
@test cfg["wide"] === Int64(9007199254740993)
@test cfg["label"] == "café 雪 😀"
@test cfg["mixed"] == [nothing, true, false, 0, 2.5, "six", []]
@test cfg["group"]["a.b"] == 7
@test cfg["absolute"][2] === 2.5
@test_throws ArgumentError loads("x: sqrt(-1)")
