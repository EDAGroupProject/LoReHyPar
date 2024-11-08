#include <hypar.hpp>
#include <cassert>
#include <iostream>
#include <unistd.h>
#include <thread>
#include <sys/resource.h>

void ipf_mt(HyPar &hp, bool &valid, long long &hop) {
    if (hp.N < 1e4) {
        hp.initial_partition();
        hp.refine();
    } else if (hp.N < 1e5) {
        hp.fast_initial_partition();
        hp.fast_refine();
    } else {
        hp.fast_initial_partition();
        hp.only_fast_refine();
    }
    hp.evaluate_summary(valid, hop, std::cout);
}

void ofr_mt(HyPar &hp, bool &valid, long long &hop) {
    hp.only_fast_refine();
    hp.evaluate_summary(valid, hop, std::cout);
}

void ghg_mt(HyPar &hp, bool &valid, long long &hop) {
    hp.greedy_hypergraph_growth(0);
    hp.evaluate_summary(valid, hop, std::cout);
}

void limit_memory_usage() {
    struct rlimit rl;
    rl.rlim_cur = 32L * 1024 * 1024 * 1024; // 32 GB
    rl.rlim_max = 32L * 1024 * 1024 * 1024; // 32 GB
    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        std::cerr << "Failed to set memory limit." << std::endl;
    }
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    limit_memory_usage();
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
    auto start = std::chrono::high_resolution_clock::now();
    if (hp.N < 1e5) {
        hp.preprocess();
        hp.coarsen();
    } else {
        hp.fast_preprocess();
        hp.fast_coarsen();
    }
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Before Coarsen: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " s" << std::endl;
    std::cout << "Node: " << hp.N << std::endl;
    if (hp.N < 1e5) {
        HyPar hp_mt[4]{};
        std::thread threads[4];
        bool valid[4], bestvalid = false;
        long long hop[4], besthop = __LONG_LONG_MAX__;
        int bestid = -1;
        while (!bestvalid) {
            std::cout << "Starting threads..." << std::endl;
            bestid = -1;
            for (int i = 0; i < 4; ++i) {
                hp_mt[i] = hp;
            }
            for (int i = 0; i < 4; ++i) {
                threads[i] = std::thread(ipf_mt, std::ref(hp_mt[i]), std::ref(valid[i]), std::ref(hop[i]));
            }
            for (int i = 0; i < 4; ++i) {
                threads[i].join();
                std::cout << "Thread " << i << " finished." << std::endl;
                if (bestvalid <= valid[i] && hop[i] < besthop) {
                    bestid = i;
                    besthop = hop[i];
                    bestvalid = valid[i];
                }
            }
            if (bestvalid) {
                std::cout << "Best thread: " << bestid << std::endl;
                hp_mt[bestid].evaluate_summary(std::cout);
                hp_mt[bestid].printOut();
                break;
            }
        }
    } else {
        hp.SCLa_propagation();
        hp.evaluate_summary(std::cout);
        HyPar hp_mt[2]{};
        std::thread threads[3];
        bool valid[3], bestvalid = false;
        long long hop[3], besthop = __LONG_LONG_MAX__;
        int bestid = -1;
        for (int r = 0; r < 2; ++r) {
            std::cout << "Starting threads round " << r << std::endl;
            for (int i = 0; i < 2; ++i) {
                hp_mt[i] = hp;
            }
            for (int i = 0; i < 2; ++i) {
                threads[i] = std::thread(ofr_mt, std::ref(hp_mt[i]), std::ref(valid[i]), std::ref(hop[i]));
            }
            if (r == 0) {
                threads[2] = std::thread(ghg_mt, std::ref(hp), std::ref(valid[2]), std::ref(hop[2]));
            }
            for (int i = 0; i < 2; ++i) {
                threads[i].join();
                std::cout << "Thread " << i << " finished." << std::endl;
                if (bestvalid <= valid[i] && hop[i] < besthop) {
                    bestid = i;
                    besthop = hop[i];
                    bestvalid = valid[i];
                }
            }
            if (bestvalid) {
                std::cout << "Best thread: " << bestid << std::endl;
                hp_mt[bestid].evaluate_summary(std::cout);
                hp_mt[bestid].printOut();
                break;
            }
            if (r == 0) {
                threads[2].join();
                hp.evaluate_summary(std::cout);
                std::cout << "Thread 2 finished." << std::endl;
            }
        }
    }
    return 0;
}