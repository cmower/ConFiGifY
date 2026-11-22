#ifdef NDEBUG
#undef NDEBUG  // Keep test assertions active in release builds.
#endif

#include <cfgy>

#include <cassert>
#include <cmath>
#include <sstream>

int main() {
    const auto cfg = cfgy::load("tests/expressions.cfgy");
    assert(cfg["myint"].as_int() == 24);
    const auto& first = cfg["mylist1"].as_list();
    assert(first.size() == 2 && first[0].as_int() == 1 && first[1].as_int() == 3);
    const auto& joined = cfg["mylist2"].as_list();
    assert(joined.size() == 4 && joined[0].as_int() == 1 && joined[1].as_int() == 2);
    assert(joined[2].as_int() == 1 && joined[3].as_int() == 3);
    const auto& words = cfg["myotherlist"].as_list();
    assert(words.size() == 2 && words[0].as_string() == "hello" && words[1].as_string() == "world");
    assert(cfg["c"]["myel1"].as_int() == 12 && cfg["c"]["myel2"].as_string() == "nice");
    assert(cfg["myfloat1"].as_float() == 12.0 && cfg["myfloat2"].as_float() == 9.0);
    const double pi = std::acos(-1.0);
    assert(std::abs(cfg["a"].as_float() - pi / 2) < 1e-12);
    assert(std::abs(cfg["b"].as_float() - 90.0) < 1e-12);
    const auto& rpy = cfg["rpy"].as_list();
    assert(rpy.size() == 3 && rpy[1].as_float() == 0.0);
    assert(std::abs(rpy[0].as_float() - pi / 2) < 1e-12);
    assert(std::abs(rpy[2].as_float() - pi) < 1e-12);

    const auto runtime = cfgy::loads("myfloat2: .25\nc:\n  myel1: 61 + 2\n'c.myel1': 7");
    int count = runtime["c"]["myel1"];
    double gain = runtime["myfloat2"];
    assert(count == 63 && gain == 0.25 && runtime["c.myel1"].as_int() == 7);
    std::ostringstream output;
    output << runtime["c"]["myel1"] << ' ' << runtime["myfloat2"];
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

    const auto math = cfgy::load("tests/conformance.cfgy");
    assert(math["rotation"].as_list().size() == 3);
    assert(std::abs(math["rotation"][1][2].as_float() - 1) < 1e-12);
    assert(std::abs(math["rotated_y"][2].as_float() + 1) < 1e-12);
    const auto matrices = cfgy::loads("a: matrix([[1, 2]])\nb: (a + a) * transpose(a)");
    assert(matrices["b"][0][0].as_float() == 10.0);
    const std::string draws = "seed: Rand.seed(7)\nx: Rand.uniform()\ny: Rand.int(1, 6)";
    const auto random = cfgy::loads(draws);
    assert(random["x"].as_float() == cfgy::loads(draws)["x"].as_float());
    assert(random["y"].as_int() >= 1 && random["y"].as_int() <= 6);
    assert(math["absolute"][0].as_int() == 3 && math["absolute"][1].as_float() == 2.5);
    assert(math["angles"][0].as_float() == 1.0 && math["angles"][1].as_float() == 1.0);
    assert(math["angles"][2].as_float() == 0.0 && math["roots"][1].as_float() == 4.0);
    assert(math["rounded"][0].as_float() == -2.0 && math["rounded"][1].as_float() == -1.0);
    for (const auto* text : {"x: abs(-9223372036854775808)", "x: sqrt(-1)", "x: sin(true)",
                             "x: abs()", "x: 1\nx: 2", "x: unknown", "x: 1/0", "x: [1] + 2",
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
