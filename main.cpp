#include <hypar.hpp>
#include <cassert>
#include <iostream>

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    assert(argc <= 2);
    LoReHyPar hypar(argc == 2 ? argv[1] : "");
    hypar.printSummary(std::cout);
    return 0;
}