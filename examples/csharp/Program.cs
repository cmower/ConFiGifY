using System;
using System.Collections.Generic;
using Cfgy;

if (args.Length != 1) throw new ArgumentException("expected a configuration path");
var cfg = Config.Load(args[0]);
Console.WriteLine(((Dictionary<string, object?>)cfg["c"]!)["myel1"]);
Console.WriteLine(cfg["myfloat2"]);
