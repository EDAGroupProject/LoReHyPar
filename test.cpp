#include <hypar.hpp>

int main() {
    std::ios::sync_with_stdio(false);
    std::string inputDir="../testcase/case04", outputFile="../testcase/case04/design.fpga.out";
    HyPar hp(inputDir, outputFile);
    auto start = std::chrono::high_resolution_clock::now();
    hp.preprocess();
    hp.coarsen();
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Coarsening time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;
    hp.fast_initial_partition();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Initial partition time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;
    hp.only_fast_refine();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Total time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;
    hp.evaluate_summary(std::cout);
    std::ofstream out(outputFile);
    hp.printOut(out);
    return 0;
}