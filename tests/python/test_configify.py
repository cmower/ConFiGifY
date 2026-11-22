import math
import unittest

import cfgy


class ParserTests(unittest.TestCase):
    def test_example(self):
        cfg = cfgy.load("example.cfgy")
        self.assertEqual(cfg.myint, 24)
        self.assertEqual(cfg.mylist1, [1, 3])
        self.assertEqual(cfg.mylist2, [1, 2, 1, 3])
        self.assertEqual(cfg.myotherlist, ["hello", "world"])
        self.assertEqual((cfg.c.myel1, cfg.c.myel2), (12, "nice"))
        self.assertIs(type(cfg.myfloat1), float)
        self.assertEqual(cfg.myfloat1, 12.0)
        self.assertIs(type(cfg.myfloat2), float)
        self.assertEqual(cfg.myfloat2, 9.0)
        self.assertAlmostEqual(cfg.a, math.pi / 2)
        self.assertAlmostEqual(cfg.b, 90.0)
        self.assertEqual(cfg.rpy, [math.pi / 2, 0.0, math.pi])

    def test_nested_scopes_and_precedence(self):
        cfg = cfgy.loads(
            'n: 10\nrows:\n  - n: 3\n    result: -(n + 1) * 2 + 8 / 2 / 2\n'
            '    label: "left # right" # outside\n  - result: n\nalias: rows\n'
        )
        self.assertEqual(cfg.alias[0].result, -6.0)
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


if __name__ == "__main__":
    unittest.main()
