#include <iomanip>
#include <iostream>
#include <random>

int
main()
{
    const uint32_t seed = 123456789u;
    std::mt19937 eng(seed);

    std::uniform_int_distribution<int> udist(1, 1000000);
    std::normal_distribution<double> ndist(0.0, 1.0);

    std::cout << "# std::mt19937 uniform_int [1,1000000]" << std::endl;
    for (int i = 0; i < 1000; ++i)
    {
        std::cout << udist(eng) << '\n';
    }

    // Reset engine for normal distribution to ensure same sequence base
    eng.seed(seed);
    std::cout << "# std::mt19937 normal mean=0 stddev=1" << std::endl;
    std::cout << std::setprecision(17);
    for (int i = 0; i < 1000; ++i)
    {
        std::cout << ndist(eng) << '\n';
    }

    return 0;
}
