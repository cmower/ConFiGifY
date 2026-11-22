using System;
using Cfgy;

if (args.Length != 1) throw new ArgumentException("expected a configuration path");
dynamic cfg = Config.Load(args[0]);
Console.WriteLine($"{cfg["recording"]["samples"]}");
Console.WriteLine($"{cfg["recording"]["interval_ms"]}");
