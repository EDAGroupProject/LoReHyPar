#include <fstream>
#include <vector>
#include <random>
#include <chrono>
#include <cmath>
#include <iostream>

inline std::mt19937 get_rng() {
    std::random_device rd;
    auto time_seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::seed_seq seed_seq{rd(), static_cast<unsigned>(time_seed)};
    return std::mt19937(seed_seq);
}

bool isPrime(int num) {
    if (num < 2) return false;
    for (int i = 2; i <= std::sqrt(num); ++i) {
        if (num % i == 0) return false;
    }
    return true;
}

int generateRandomPrime(int max_p) {
    std::mt19937 rng(get_rng());
    std::uniform_int_distribution<int> dist(2, max_p);

    int candidate;
    do {
        candidate = dist(rng);
    } while (!isPrime(candidate));

    return candidate;
}

int main() {
    size_t num_hashes = std::sqrt(5e6) * 10;
    int max_p = num_hashes * std::log(num_hashes);
    std::vector<int> a_values;
    std::vector<int> b_values;
    std::vector<int> p_values;

    std::mt19937 rng(get_rng());
    for (size_t i = 0; i < num_hashes; ++i) {
        int p = generateRandomPrime(max_p);
        p_values.push_back(p);
        
        std::uniform_int_distribution<int> dist_a(1, p - 1);
        std::uniform_int_distribution<int> dist_b(0, p - 1);
        
        a_values.push_back(dist_a(rng));
        b_values.push_back(dist_b(rng));
    }

    std::ofstream outfile("../LSHConstants.hpp");
    outfile << "#ifndef _LSH_CONSTANTS_HPP\n";
    outfile << "#define _LSH_CONSTANTS_HPP\n\n";
    outfile << "const int NUM_HASHES = " << num_hashes << ";\n";
    outfile << "const int A_VALUES[] = { ";
    for (size_t i = 0; i < a_values.size(); ++i) {
        outfile << a_values[i] << (i < a_values.size() - 1 ? ", " : " ");
    }
    outfile << "};\n";
    outfile << "const int B_VALUES[] = { ";
    for (size_t i = 0; i < b_values.size(); ++i) {
        outfile << b_values[i] << (i < b_values.size() - 1 ? ", " : " ");
    }
    outfile << "};\n";
    outfile << "const int P_VALUES[] = { ";
    for (size_t i = 0; i < p_values.size(); ++i) {
        outfile << p_values[i] << (i < p_values.size() - 1 ? ", " : " ");
    }
    outfile << "};\n\n";
    outfile << "#endif // LSH_CONSTANTS_HPP\n";
    outfile.close();

    return 0;
}
