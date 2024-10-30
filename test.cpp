#include <hypar.hpp>

int main() {
    std::ios::sync_with_stdio(false);
    std::string inputDir="../testcase/case01", outputFile="../testcase/case01/design.fpga.out";
    HyPar hp(inputDir, outputFile);
    std::cout << "Read done" << std::endl;

    // 输出初始图结构
    std::cout << "Initial Graph:" << std::endl;
    hp.debugUnconnectedGraph(std::cout);
    hp.preprocess();
    std::cout << "Preprocess done" << std::endl;

    return 0;
}