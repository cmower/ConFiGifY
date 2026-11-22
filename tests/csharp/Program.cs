using System;
using System.Collections.Generic;
using Cfgy;

var cfg = Config.Load(args[0]);
var mixed = (object?[])cfg["mixed"]!;
if (!Equals(cfg["wide"], 9007199254740993L) || !Equals(cfg["label"], "café 雪 😀") ||
    !Equals(((Dictionary<string, object?>)cfg["group"]!)["a.b"], 7L) ||
    mixed[0] != null || !Equals(mixed[1], true) || ((object?[])mixed[6]!).Length != 0 ||
    !Equals(((object?[])cfg["absolute"]!)[1], 2.5))
    throw new Exception("unexpected configuration");
try { Config.Loads("x: sqrt(-1)"); throw new Exception("expected a parse error"); }
catch (FormatException e) when (e.Message.Contains("<string>:1:4:")) {}
