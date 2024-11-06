#include <hypar.hpp>
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <thread>

void run(HyPar &hp, bool &valid, long long &hop) {
    hp.run_after_coarsen(valid, hop);
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    assert(argc == 5);
    std::string inputDir, outputFile;
    int opt;
    while ((opt = getopt(argc, argv, "t:s:")) != -1) {
        switch (opt) {
            case 't':
                inputDir = optarg;
                break;
            case 's':
                outputFile = optarg;
                break;
            default:
                std::cerr << "Usage: " << argv[0] << " -t <input directory> -s <output file>" << std::endl;
                return 1;
        }
    }
    if (inputDir.empty() || outputFile.empty()) {
        std::cerr << "Usage: " << argv[0] << " -t <input directory> -s <output file>" << std::endl;
        return 1;
    }
    HyPar hp(inputDir, outputFile);
    HyPar hp_mt[4];
    hp.preprocess();
    hp.coarsen();
    std::thread threads[4];
    bool valid[4], bestvalid = false;
    long long hop[4], besthop = __LONG_LONG_MAX__;
    int bestid = -1;
    while (!bestvalid) {
        std::cout << "Starting threads..." << std::endl;
        bestvalid = false;
        besthop = __LONG_LONG_MAX__;
        for (int i = 0; i < 4; ++i) {
            hp_mt[i] = hp;
        }
        for (int i = 0; i < 4; ++i) {
            threads[i] = std::thread(run, std::ref(hp_mt[i]), std::ref(valid[i]), std::ref(hop[i]));
        }
        std::cout << "Waiting for threads to finish..." << std::endl;
        for (int i = 0; i < 4; ++i) {
            std::cout << "Thread " << i << " finished." << std::endl;
            threads[i].join();
            if (bestvalid <= valid[i] && hop[i] < besthop) {
                bestid = i;
                besthop = hop[i];
                bestvalid = valid[i];
            }
        }
    }
    hp_mt[bestid].evaluate_summary(std::cout);
    hp_mt[bestid].printOut();
    return 0;
}