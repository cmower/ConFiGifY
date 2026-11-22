#include <cfgy>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config-file>\n";
        return 2;
    }
    try {
        const auto cfg = cfgy::load(argv[1]);
        const double gain = cfg["myfloat2"];
        std::cout << cfg.at("c.myel1") << '\n';
        std::cout.precision(1);
        std::cout << std::fixed << gain << '\n';
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
