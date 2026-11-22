#include <cfgy>
#include "example_config.hpp"
#include "test_config.hpp"

#include <cassert>
#include <cmath>
#include <sstream>
#include <type_traits>

int main() {
    const auto cfg = cfgy::load<TestConfig>("example.cfgy");
    static_assert(std::is_same_v<decltype(cfg.c.myel1), std::int64_t>);
    static_assert(std::is_same_v<decltype(cfg.myfloat2), double>);
    assert(cfg.myint == 24);
    assert((cfg.mylist1 == std::vector<std::int64_t>{1, 3}));
    assert((cfg.mylist2 == std::vector<std::int64_t>{1, 2, 1, 3}));
    assert((cfg.myotherlist == std::vector<std::string>{"hello", "world"}));
    assert(cfg.c.myel1 == 12 && cfg.c.myel2 == "nice");
    assert(cfg.myfloat1 == 12.0 && cfg.myfloat2 == 9.0);
    const double pi = std::acos(-1.0);
    assert(std::abs(cfg.a - pi / 2) < 1e-12);
    assert(std::abs(cfg.b - 90.0) < 1e-12);
    assert(cfg.rpy.size() == 3 && cfg.rpy[1] == 0.0);
    assert(std::abs(cfg.rpy[0] - pi / 2) < 1e-12);
    assert(std::abs(cfg.rpy[2] - pi) < 1e-12);

    try {
        cfgy::loads<TestConfig>("a: 1.5\nb: 2.5\nc:\n  myel1: 'wrong type'", "changed.cfgy");
        assert(false && "expected a schema mismatch");
    } catch (const cfgy::ConfigError& error) {
        assert(std::string(error.what()) == "changed.cfgy: type mismatch at c.myel1");
    }

    const auto runtime = cfgy::loads<ExampleConfig>("myfloat2: .25\nc:\n  myel1: 61 + 2");
    int count = runtime.c.myel1;
    double gain = runtime.myfloat2;
    assert(count == 63 && gain == 0.25);
    std::ostringstream output;
    output << runtime.c.myel1 << ' ' << runtime.myfloat2;
    assert(output.str() == "63 0.25");

    const auto nested = cfgy::loads(
        "n: 10\nrows:\n  - n: 3\n    result: -(n + 1) * 2 + 8 / 2 / 2\n"
        "    label: 'left # right' # outside\n  - result: n\nalias: rows\n"
        "a:\n  b: 2\nc: a.b + 1");
    double result = nested["alias"][0]["result"];
    assert(result == -6.0);
    assert(nested["rows"][0]["label"].as_string() == "left # right");
    int second = nested["rows"][1]["result"];
    int dotted = nested.at("a.b");
    assert(second == 10 && dotted == 2);

    for (const auto* text : {"x: 1\nx: 2", "x: unknown", "x: 1/0", "x: [1] + 2",
                             "x: torad()", "x: [1, 2", "x: 2 *", "x: \"unfinished",
                             "x:\n  y: 1\n z: 2", "x: 9223372036854775807 + 1"}) {
        try {
            cfgy::loads(text, "test.cfgy");
            assert(false && "expected a ConfigError");
        } catch (const cfgy::ConfigError& error) {
            assert(std::string(error.what()).rfind("test.cfgy:", 0) == 0);
        }
    }
}
