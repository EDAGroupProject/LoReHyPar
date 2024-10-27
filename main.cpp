#include <hypar.hpp>
#include <cassert>
#include <iostream>
#include <unistd.h>

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    assert(argc == 5);
    std::string inputDir, outputFile;
    int opt;
    while ((opt = getopt(argc, argv, "s:t:")) != -1) {
        switch (opt) {
            case 's':
                inputDir = optarg;
                break;
            case 't':
                outputFile = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -s <input directory> -t <output file>" << std::endl;
                return 1;
        }
    }
    if (inputDir.empty() || outputFile.empty()) {
        std::cerr << "Usage: " << argv[0] << " -s <input directory> -t <output file>" << std::endl;
        return 1;
    }
    HyPar hp(inputDir, outputFile);
    hp.run();
    return 0;
}