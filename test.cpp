#include <hypar.hpp>

int main() {
    std::ios::sync_with_stdio(false);
    std::string inputDir="../testcase/case02", outputFile="../testcase/case02/design.fpga.out";
    HyPar hp(inputDir, outputFile);
    std::cout << "Read done" << std::endl;
    hp.preprocess();
    std::cout << "Preprocess done" << std::endl;
    hp.coarsen();
    std::cout << "Coarsen done" << std::endl;
    hp.initial_partition();
    std::cout << "Initial partition done" << std::endl;
    // hp.refine();
    // std::cout << "Refine done" << std::endl;
    // hp.evaluate_summary(std::cout);
    return 0;
}