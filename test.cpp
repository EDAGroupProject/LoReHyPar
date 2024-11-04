#include <hypar.hpp>

int main() {
    std::ios::sync_with_stdio(false);
    std::string inputDir="../testcase/case01", outputFile="../testcase/case01/design.fpga.out";
    HyPar hp(inputDir, outputFile);
    hp.fast_run();
    // hp.evaluate_summary(std::cout);
    return 0;
}