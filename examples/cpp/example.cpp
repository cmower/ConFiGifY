#include <cfgy>
#include <iostream>

int main(int argc, char* argv[]) {
    if (argc != 2) {
        std::cerr << "Usage: " << argv[0] << " <config-file>\n";
        return 2;
    }
    const auto cfg = cfgy::load(argv[1]);
    const double gain = cfg["myfloat2"];
    std::cout << cfg["c"]["myel1"] << '\n';
    std::cout << gain << '\n';
}
