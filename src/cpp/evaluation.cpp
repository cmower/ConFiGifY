#include "evaluation.hpp"

#include <algorithm>
#include <array>
#include <cmath>
#include <functional>
#include <stdexcept>
#include <utility>

namespace cfgy::detail {

bool is_number(const Eval& value) {
    return std::holds_alternative<Integer>(value.data) || std::holds_alternative<double>(value.data);
}

double to_double(const Eval& value) {
    if (const auto* n = std::get_if<Integer>(&value.data)) return static_cast<double>(*n);
    if (const auto* n = std::get_if<double>(&value.data)) return *n;
    throw std::domain_error("expected a number");
}

Value materialize(Eval value) {
    return std::visit([](auto&& item) -> Value {
        using T = std::decay_t<decltype(item)>;
        if constexpr (std::is_same_v<T, Eval::Map>) {
            Value::Map fields;
            for (auto& [key, child] : item) fields.emplace_hint(fields.end(), key, materialize(std::move(child)));
            return Value{std::move(fields)};
        } else if constexpr (std::is_same_v<T, Eval::List>) {
            Value::List items;
            items.reserve(item.size());
            for (auto& child : item) items.push_back(materialize(std::move(child)));
            return Value{std::move(items)};
        } else if constexpr (std::is_same_v<T, Matrix>) {
            Value::List rows;
            rows.reserve(item.size());
            for (const auto& row : item) {
                Value::List cells;
                cells.reserve(row.size());
                for (double cell : row) cells.push_back(Value{cell});
                rows.push_back(Value{std::move(cells)});
            }
            return Value{std::move(rows)};
        } else {
            return Value{std::move(item)};
        }
    }, std::move(value.data));
}

namespace {

double finite(double value) {
    if (!std::isfinite(value)) throw std::domain_error("float out of range");
    return value;
}

bool is_row_list(const Eval& value) {
    const auto* rows = std::get_if<Eval::List>(&value.data);
    return rows && !rows->empty() && std::holds_alternative<Eval::List>(rows->front().data);
}

Matrix read_rows(const Eval& value) {
    const auto* rows = std::get_if<Eval::List>(&value.data);
    if (!rows || rows->empty()) throw std::domain_error("expected a nonempty list of matrix rows");
    Matrix result;
    result.reserve(rows->size());
    for (const auto& row : *rows) {
        const auto* cells = std::get_if<Eval::List>(&row.data);
        if (!cells || cells->empty() || (!result.empty() && cells->size() != result.front().size()))
            throw std::domain_error("matrix rows must be nonempty and the same length");
        auto& numbers = result.emplace_back();
        numbers.reserve(cells->size());
        for (const auto& cell : *cells) numbers.push_back(to_double(cell));
    }
    return result;
}

Eval scale(Matrix matrix, char op, double scalar) {
    if (op == '/' && scalar == 0) throw std::domain_error("division by zero");
    for (auto& row : matrix)
        for (auto& cell : row) cell = finite(op == '/' ? cell / scalar : cell * scalar);
    return Eval{std::move(matrix)};
}

template<class Operation>
Eval elementwise(Matrix left, const Matrix& right, Operation combine) {
    if (left.size() != right.size() || left.front().size() != right.front().size())
        throw std::domain_error("element-wise operations require matrices of the same shape");
    for (std::size_t i = 0; i < left.size(); ++i) {
        for (std::size_t j = 0; j < left[i].size(); ++j)
            left[i][j] = finite(combine(left[i][j], right[i][j]));
    }
    return Eval{std::move(left)};
}

Eval matrix_product(const Matrix& a, const Matrix& b) {
    if (a.front().size() != b.size()) throw std::domain_error("matrix product dimensions do not match");
    Matrix result(a.size(), std::vector<double>(b.front().size()));
    for (std::size_t i = 0; i < a.size(); ++i) {
        for (std::size_t k = 0; k < b.size(); ++k) {
            for (std::size_t j = 0; j < b[k].size(); ++j) {
                result[i][j] += a[i][k] * b[k][j];
            }
        }
        for (double cell : result[i]) finite(cell);
    }
    return Eval{std::move(result)};
}

Eval apply_vector(const Matrix& matrix, const Eval::List& vector) {
    if (matrix.front().size() != vector.size()) throw std::domain_error("matrix and vector dimensions do not match");
    std::vector<double> numbers;
    for (const auto& cell : vector) numbers.push_back(to_double(cell));
    Eval::List result;
    for (const auto& row : matrix) {
        double sum = 0;
        for (std::size_t k = 0; k < row.size(); ++k) sum += row[k] * numbers[k];
        result.push_back(Eval{finite(sum)});
    }
    return Eval{std::move(result)};
}

template<std::size_t N>
std::array<double, N> components(const Eval::List& args, std::string_view name) {
    const auto* list = args.size() == 1 ? std::get_if<Eval::List>(&args.front().data) : nullptr;
    const auto& values = list ? *list : args;
    if (values.size() != N)
        throw std::domain_error(std::string(name) + " expects " + std::to_string(N) + " numeric components");
    std::array<double, N> result;
    std::transform(values.begin(), values.end(), result.begin(), to_double);
    return result;
}

} // namespace

bool is_matrix_operand(const Eval& value) {
    return std::holds_alternative<Matrix>(value.data) || is_row_list(value);
}

Matrix as_matrix(Eval value) {
    if (auto* matrix = std::get_if<Matrix>(&value.data)) return std::move(*matrix);
    return read_rows(value);
}

Matrix transpose(const Matrix& matrix) {
    Matrix result(matrix.front().size(), std::vector<double>(matrix.size()));
    for (std::size_t i = 0; i < matrix.size(); ++i)
        for (std::size_t j = 0; j < matrix[i].size(); ++j) result[j][i] = matrix[i][j];
    return result;
}

Eval matrix_operation(char op, Eval left, Eval right) {
    if (is_row_list(left)) left.data = read_rows(left);
    if (is_row_list(right)) right.data = read_rows(right);

    auto* a = std::get_if<Matrix>(&left.data);
    auto* b = std::get_if<Matrix>(&right.data);
    if (op == '@') {
        if (a && b) return matrix_product(*a, *b);
        if (a && std::holds_alternative<Eval::List>(right.data)) return apply_vector(*a, right.as_list());
        throw std::domain_error("'@' expects a matrix on the left and a matrix or vector on the right");
    }
    if (a && b) {
        if (op == '+') return elementwise(std::move(*a), *b, std::plus<double>{});
        if (op == '-') return elementwise(std::move(*a), *b, std::minus<double>{});
        if (op == '*') return elementwise(std::move(*a), *b, std::multiplies<double>{});
        throw std::domain_error("expected a scalar divisor");
    }
    if (a && is_number(right) && (op == '*' || op == '/')) return scale(std::move(*a), op, to_double(right));
    if (b && is_number(left) && op == '*') return scale(std::move(*b), op, to_double(left));
    throw std::domain_error("invalid matrix operands");
}

Eval rotation(std::string_view name, const Eval::List& args) {
    if (name == "Rot.from_rpy") {
        auto angles = components<3>(args, name);
        for (double& angle : angles) angle = to_radians(angle);
        const auto [roll, pitch, yaw] = angles;
        const double cr = std::cos(roll), sr = std::sin(roll);
        const double cp = std::cos(pitch), sp = std::sin(pitch);
        const double cy = std::cos(yaw), sy = std::sin(yaw);
        // Active, right-handed rotation Rz(yaw) * Ry(pitch) * Rx(roll).
        return Eval{Matrix{{cy*cp, cy*sp*sr - sy*cr, cy*sp*cr + sy*sr},
                           {sy*cp, sy*sp*sr + cy*cr, sy*sp*cr - cy*sr},
                           {-sp,   cp*sr,            cp*cr}}};
    }
    if (name == "Rot.from_quat") {
        auto q = components<4>(args, name);
        double largest = 0;
        for (double v : q) largest = std::max(largest, std::abs(v));
        if (largest == 0) throw std::domain_error("zero quaternion has no rotation");
        // Scaling first avoids overflow and underflow when normalizing.
        double sum = 0;
        for (auto& v : q) {
            v /= largest;
            sum += v * v;
        }
        const auto [x, y, z, w] = q;
        const double s = 2 / sum;
        return Eval{Matrix{{1 - s*(y*y + z*z), s*(x*y - z*w), s*(x*z + y*w)},
                           {s*(x*y + z*w), 1 - s*(x*x + z*z), s*(y*z - x*w)},
                           {s*(x*z - y*w), s*(y*z + x*w), 1 - s*(x*x + y*y)}}};
    }
    throw std::domain_error("unknown function '" + std::string(name) + "'");
}

std::mt19937_64& Random::engine() {
    if (!state_) {
        try {
            state_.emplace(std::random_device{}());
        } catch (const std::runtime_error& error) {
            throw std::domain_error(std::string("could not seed random generator: ") + error.what());
        }
    }
    return *state_;
}

Eval Random::call(std::string_view name, const Eval::List& args, std::mt19937_64* stream) {
    if (name == "Rand.seed") {
        if (args.size() != 1 || !std::holds_alternative<Integer>(args[0].data))
            throw std::domain_error("Rand.seed expects an integer");
        const auto seed = args[0].as_int();
        state_.emplace(static_cast<std::uint64_t>(seed));
        return Eval{seed, std::make_shared<std::mt19937_64>(*state_)};
    }

    auto sample = [&](auto draw) -> Eval {
        if (!stream) return Eval{draw(engine())};
        return Eval{Eval::List{Eval{draw(*stream)}, Eval{draw(*stream)}}};
    };
    if (name == "Rand.uniform") {
        if (!args.empty() && args.size() != 2) throw std::domain_error("Rand.uniform expects zero or two bounds");
        const double lo = args.empty() ? 0 : to_double(args[0]);
        const double hi = args.empty() ? 1 : to_double(args[1]);
        if (!(lo < hi)) throw std::domain_error("Rand.uniform requires low < high");
        return sample([lo, hi](auto& generator) {
            const double u = std::generate_canonical<double, 53>(generator);
            // Interpolate across zero without overflowing hi - lo.
            const double draw = lo < 0 && hi > 0 ? (1 - u) * lo + u * hi : lo + u * (hi - lo);
            return std::min(draw, std::nextafter(hi, lo));
        });
    }
    if (name == "Rand.normal") {
        if (!args.empty() && args.size() != 2) throw std::domain_error("Rand.normal expects zero or two parameters");
        const double mean = args.empty() ? 0 : to_double(args[0]);
        const double deviation = args.empty() ? 1 : to_double(args[1]);
        if (!(deviation > 0)) throw std::domain_error("Rand.normal requires a positive standard deviation");
        return sample([mean, deviation](auto& generator) {
            return finite(std::normal_distribution<double>(mean, deviation)(generator));
        });
    }
    if (name == "Rand.int") {
        if (args.size() != 2 || !std::holds_alternative<Integer>(args[0].data) ||
            !std::holds_alternative<Integer>(args[1].data))
            throw std::domain_error("Rand.int expects two integer bounds");
        const Integer lo = args[0].as_int(), hi = args[1].as_int();
        if (lo > hi) throw std::domain_error("Rand.int requires low <= high");
        return sample(std::uniform_int_distribution<Integer>(lo, hi));
    }
    throw std::domain_error("unknown function '" + std::string(name) + "'");
}

} // namespace cfgy::detail
