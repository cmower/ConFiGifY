#pragma once

#include <cfgy>
#include <memory>
#include <optional>
#include <random>
#include <string_view>

namespace cfgy::detail {

using Integer = std::int64_t;
using Matrix = std::vector<std::vector<double>>;
inline constexpr double pi = 3.14159265358979323846;
inline double to_radians(double degrees) { return degrees * (pi / 180.0); }

// A matrix retains its arithmetic meaning through references during evaluation.
// At the binding boundary it becomes an ordinary list of rows.
struct Eval {
    using List = std::vector<Eval>;
    using Map = std::map<std::string, Eval>;
    std::variant<std::nullptr_t, bool, Integer, double, std::string, List, Map, Matrix> data;
    // Rand.seed stores an integer in data. References to it share the draw sequence.
    std::shared_ptr<std::mt19937_64> random_stream = nullptr;

    Integer as_int() const { return std::get<Integer>(data); }
    double as_float() const { return std::get<double>(data); }
    const List& as_list() const { return std::get<List>(data); }
};

Value materialize(Eval value);
bool is_number(const Eval& value);
double to_double(const Eval& value);
bool is_matrix_operand(const Eval& value);
Matrix as_matrix(Eval value);
Matrix diagonal(const Eval::List& args);
Matrix transpose(const Matrix& matrix);
Eval matrix_operation(char op, Eval left, Eval right);
Eval rotation(std::string_view name, const Eval::List& args);

class Random {
public:
    Eval call(std::string_view name, const Eval::List& args, std::mt19937_64* stream);
private:
    std::optional<std::mt19937_64> state_;
    std::mt19937_64& engine();
};

} // namespace cfgy::detail
