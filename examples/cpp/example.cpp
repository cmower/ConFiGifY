#include <cfgy>
#include "example_config.hpp"

#include <iomanip>
#include <iostream>

int main() {
    const auto cfg = cfgy::load<ExampleConfig>("example.cfgy");
    std::cout << cfg.c.myel1 << '\n';  // 12
    std::cout << std::fixed << std::setprecision(1)
              << cfg.myfloat2 << '\n';  // 9.0
}
