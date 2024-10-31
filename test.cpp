#include <hypar.hpp>

int main() {
    std::ios::sync_with_stdio(false);
    std::string inputDir="../testcase/case04", outputFile="../testcase/case04/design.fpga.out";
    auto start = std::chrono::high_resolution_clock::now();
    HyPar hp(inputDir, outputFile);
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Read: " << duration.count() << "s" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    hp.preprocess();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Preprocess: " << duration.count() << "s" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    hp.coarsen();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Coarsen: " << duration.count() << "s" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    hp.initial_partition();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Initial partition: " << duration.count() << "s" << std::endl;
    start = std::chrono::high_resolution_clock::now();
    hp.refine();
    end = std::chrono::high_resolution_clock::now();
    duration = std::chrono::duration_cast<std::chrono::seconds>(end - start);
    std::cout << "Refine: " << duration.count() << "s" << std::endl;
    hp.evaluate_summary(std::cout);
    return 0;
}