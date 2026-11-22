#include <cfgy>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config-file>\n";
        return 2;
    }
    const auto cfg = cfgy::load(argv[1]);
    const double interval_ms = cfg["recording"]["interval_ms"];
    std::cout << cfg["recording"]["samples"] << '\n';
    std::cout << interval_ms << '\n';
}
