import math
import unittest

import cfgy


class ParserTests(unittest.TestCase):
    def test_native_math_and_values(self):
        cfg = cfgy.load("tests/conformance.cfgy")
        self.assertEqual(cfg.absolute, [3, 2.5, 0])
        self.assertIs(type(cfg.absolute[0]), int)
        self.assertEqual(cfg.angles, [1.0, 1.0, 0.0])
        self.assertEqual(cfg.roots, [3.0, 4.0])
        self.assertEqual(cfg.wide, 9007199254740993)
        self.assertEqual(cfg.label, "café 雪 😀")
        self.assertEqual(cfg.mixed, [None, True, False, 0, 2.5, "six", []])
        self.assertEqual(cfg.group["a.b"], 7)
        self.assertEqual(cfgy.loads('s: "a\x00b"').s, "a\x00b")
        with self.assertRaisesRegex(cfgy.ConfigError, "bad:1:4:"):
            cfgy.loads("x: sqrt(-1)", "bad")

    def test_example(self):
        cfg = cfgy.load("examples/example.cfgy")
        self.assertEqual(cfg.sensor.channels, ["ax", "ay", "az", "gx", "gy", "gz"])
        self.assertAlmostEqual(cfg.mount.tilt_rad, math.pi / 6)
        self.assertAlmostEqual(cfg.trigger.z_threshold_m_s2, 9.80665 * math.cos(math.pi / 6) + 2)
        self.assertEqual(cfg.recording.pretrigger_samples, 400)
        self.assertEqual(cfg.recording.samples, 800)
        self.assertEqual(cfg.recording.interval_ms, 2.5)
        x, y = cfg.simulation.lateral_noise_m_s2
        self.assertEqual(cfg.simulation.sensor_pulse_m_s2, [x, y, 2])
        c, s = math.cos(cfg.mount.tilt_rad), math.sin(cfg.mount.tilt_rad)
        body = cfg.simulation.body_pulse_m_s2
        self.assertAlmostEqual(body[0], c * 1.01 * x + s * 0.99 * 2)
        self.assertAlmostEqual(body[1], y)
        self.assertAlmostEqual(body[2], -s * 1.01 * x + c * 0.99 * 2)

    def test_expressions(self):
        cfg = cfgy.load("tests/expressions.cfgy")
        self.assertEqual(cfg.myint, 24)
        self.assertEqual(cfg.mylist1, [1, 3])
        self.assertEqual(cfg["mylist2"], [1, 2, 1, 3])
        self.assertEqual(cfg.myotherlist, ["hello", "world"])
        self.assertEqual((cfg["c"]["myel1"], cfg["c"]["myel2"]), (12, "nice"))
        self.assertIs(type(cfg.myfloat1), float)
        self.assertEqual(cfg.myfloat1, 12.0)
        self.assertIs(type(cfg["myfloat2"]), float)
        self.assertEqual(cfg["myfloat2"], 9.0)
        self.assertAlmostEqual(cfg.a, math.pi / 2)
        self.assertAlmostEqual(cfg.b, 90.0)
        self.assertEqual(cfg.rpy, [math.pi / 2, 0.0, math.pi])

    def test_nested_scopes_and_precedence(self):
        cfg = cfgy.loads(
            'n: 10\nrows:\n  - n: 3\n    result: -(n + 1) * 2 + 8 / 2 / 2\n'
            '    label: "left # right" # outside\n  - result: n\nalias: rows\n"alias.0": 7\n'
        )
        self.assertEqual(cfg["alias"][0]["result"], -6.0)
        self.assertEqual(cfg["alias.0"], 7)
        self.assertEqual(cfg.rows[0].label, "left # right")
        self.assertEqual(cfg.rows[1].result, 10)
        self.assertEqual(cfgy.loads("a:\n  b: 2\nc: a.b + 1").c, 3)

    def test_invalid_input_reports_location(self):
        for text in ["x: 1\nx: 2", "x: unknown", "x: 1/0", "x: [1] + 2",
                     "x: torad()", "x: [1, 2", "x: 2 *", 'x: "unfinished',
                     "x:\n  y: 1\n z: 2", "x: 9223372036854775807 + 1"]:
            with self.subTest(text=text), self.assertRaisesRegex(
                cfgy.ConfigError, r"^test\.cfgy:\d+:\d+:"
            ):
                cfgy.loads(text, source="test.cfgy")

    def test_matrix_arithmetic(self):
        cfg = cfgy.loads("""
cal:
  a: matrix([[1, 2, 3], [4, 5, 6]])
alias: cal.a
b: [[2, 0], [0, 1], [1, -1]]
product: alias @ b
vector: alias @ [2, 0, 1]
weighted: alias * [[1, 0, -1], [2, 3, 4]]
scaled: 3 * alias / 2
sum: alias + cal.a * 2
difference: sum - alias
negative: -alias
joined: [[1, 2]] + [[3, 4]]
row_product: [[1, 2]] @ [[3], [4]]
columns: transpose(alias)
""")
        self.assertEqual(cfg.product, [[5, -1], [14, -1]])
        self.assertEqual(cfg.vector, [5, 14])
        self.assertEqual(cfg.weighted, [[1, 0, -3], [8, 15, 24]])
        self.assertEqual(cfg.scaled, [[1.5, 3, 4.5], [6, 7.5, 9]])
        self.assertEqual(cfg["sum"], [[3, 6, 9], [12, 15, 18]])
        self.assertEqual(cfg.difference, [[2, 4, 6], [8, 10, 12]])
        self.assertEqual(cfg.negative, [[-1, -2, -3], [-4, -5, -6]])
        self.assertEqual(cfg.joined, [[1, 2], [3, 4]])
        self.assertEqual(cfg.row_product, [[11]])
        self.assertEqual(cfg.columns, [[1, 4], [2, 5], [3, 6]])
        self.assertIs(type(cfg.alias[0][0]), float)

    def test_matrix_product_operators(self):
        cfg = cfgy.loads("""
a: [[1, 2], [3, 4]]
b: [[5, 6], [7, 8]]
cells: a * b
product: a @ b
mixed: a * b @ a
sum: a + a @ b
""")
        self.assertEqual(cfg.cells, [[5, 12], [21, 32]])
        self.assertEqual(cfg.product, [[19, 22], [43, 50]])
        self.assertEqual(cfg.mixed, [[41, 58], [117, 170]])
        self.assertEqual(cfg["sum"], [[20, 24], [46, 54]])

    def test_rotation_conventions(self):
        cfg = cfgy.load("tests/conformance.cfgy")
        expected = [[1, 0, 0], [0, 0, 1], [0, -1, 0]]
        for row, expected_row in zip(cfg.rotation, expected):
            for actual, target in zip(row, expected_row):
                self.assertAlmostEqual(actual, target)
        for magnitude in ("1", "1e300", "1e-300"):
            cfg = cfgy.loads(f"v: Rot.from_quat([0, 0, {magnitude}, {magnitude}]) @ [1, 0, 0]")
            self.assertEqual(cfg.v, [0, 1, 0])
        # RPY applies roll, then pitch, then yaw to a column vector.
        cfg = cfgy.loads("v: Rot.from_rpy(90, 90, 90) @ [0, 1, 0]")
        for actual, target in zip(cfg.v, [0, 1, 0]):
            self.assertAlmostEqual(actual, target)
        cfg = cfgy.loads("Rot:\n  from_rpy: 7\nv: Rot.from_rpy")
        self.assertEqual(cfg.v, 7)

    def test_random_draws(self):
        draws = """
uniform: Rand.uniform(-3, 2)
integer: Rand.int(-5, 3)
normal: Rand.normal(10, 2)
unit: Rand.uniform()
standard: Rand.normal()
alias: uniform
"""
        text = "seed: Rand.seed(1984)\n" + draws
        cfg = cfgy.loads(text)
        repeat = cfgy.loads(text)
        for key in ("uniform", "integer", "normal", "unit", "standard"):
            self.assertEqual(cfg[key], repeat[key])
        self.assertNotEqual(cfg.uniform, cfgy.loads(text.replace("1984", "1985")).uniform)
        for cfg in (cfg, cfgy.loads(draws)):
            self.assertTrue(-3 <= cfg.uniform < 2 and 0 <= cfg.unit < 1)
            self.assertTrue(-5 <= cfg.integer <= 3)
            self.assertIs(type(cfg.integer), int)
            self.assertTrue(math.isfinite(cfg.normal) and math.isfinite(cfg.standard))
            self.assertEqual(cfg["alias"], cfg.uniform)
        reset = cfgy.loads("s: Rand.seed(0)\na: Rand.uniform()\nt: Rand.seed(0)\nb: Rand.uniform()")
        self.assertEqual(reset.a, reset.b)
        self.assertEqual(cfgy.loads("x: Rand.int(-5, -5)").x, -5)

    def test_seeded_pairs_and_independent_streams(self):
        cfg = cfgy.loads("""
seeds:
  first: Rand.seed(42)
  second: Rand.seed(12)
a: Rand.uniform[seeds.first](-1, 1)
b: Rand.normal[seeds.second](2, 0.5)
alias: seeds.first
c: Rand.int[alias](-4, 4)
repeat: Rand.seed(42)
d: Rand.uniform[repeat](-1, 1)
e: Rand.int[repeat](-4, 4)
f: Rand.uniform[seeds.first]()
g: Rand.normal[seeds.second]()
default_draw: Rand.uniform(-1, 1)
""")
        first = cfgy.loads("""
s: Rand.seed(42)
u: [Rand.uniform(-1, 1), Rand.uniform(-1, 1)]
i: [Rand.int(-4, 4), Rand.int(-4, 4)]
unit: [Rand.uniform(), Rand.uniform()]
""")
        second = cfgy.loads("""
s: Rand.seed(12)
n: [Rand.normal(2, 0.5), Rand.normal(2, 0.5)]
standard: [Rand.normal(), Rand.normal()]
""")
        self.assertIs(type(cfg.alias), int)
        self.assertEqual(cfg.alias, 42)
        self.assertEqual(cfg.a, cfg.d)
        self.assertEqual(cfg.c, cfg.e)
        self.assertEqual((cfg.a, cfg.c, cfg.f), (first.u, first.i, first.unit))
        self.assertEqual((cfg.b, cfg.g), (second.n, second.standard))
        self.assertEqual(cfg.default_draw, first.u[0])
        self.assertTrue(all(type(n) is int for n in cfg.c))

    def test_invalid_seed_selectors(self):
        for expr in ["Rand.uniform[missing]()", "Rand.uniform[42]()", "Rand.uniform[true]()",
                     "Rand.uniform[]()", "Rand.uniform[s, s]()", "Rand.uniform[s()",
                     "Rand.uniform[s]", "Rand.seed[s](0)", "Rot.from_rpy[s](0, 0, 0)",
                     "Rand.uniform[s](1, 1)", "Rand.int[s](0, 1.5)", "Rand.normal[s](0, 0)"]:
            with self.subTest(expr=expr), self.assertRaisesRegex(cfgy.ConfigError, r"^bad:2:\d+:"):
                cfgy.loads("s: Rand.seed(0)\nx: " + expr, source="bad")

    def test_invalid_matrix_rotation_and_random_arguments(self):
        invalid = [
            "matrix()", "matrix([])", "matrix([[]])", "matrix([[1], [2, 3]])",
            "matrix([[true]])", "matrix([[1]]) + matrix([[1, 2]])",
            "[[1, 2]] * [[3], [4]]", "[[1, 2]] @ [[3, 4]]", "matrix([[1, 2]]) * [1, 2]",
            "[[1, 2]] @ [1]", "1 @ 2", "1.0 @ 2.0", "[[2]] @ 3", "3 @ [[2]]",
            "[[1e300]] * [[1e300]]", "[[1e300]] @ [[1e300]]",
            "matrix([[1]]) / 0", "matrix([[1e300]]) * 1e300",
            "Rot.from_quat(0, 0, 0, 0)", "Rot.from_rpy(1, 2)", "Rot.from_quat([0, 0, 1, 'x'])",
            "Rot.unknown()", "Rand.seed(1.5)", "Rand.uniform(1)", "Rand.uniform(1, 1)",
            "Rand.normal(0, 0)", "Rand.int(3, 2)", "Rand.int(0, 1.5)",
        ]
        for expr in invalid:
            with self.subTest(expr=expr), self.assertRaisesRegex(cfgy.ConfigError, r"^bad:1:\d+:"):
                cfgy.loads("x: " + expr, "bad")


if __name__ == "__main__":
    unittest.main()
