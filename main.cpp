#include <hypar.hpp>
#include <unistd.h>
#include <memory>
#include <sys/resource.h>

const size_t MEMORY_LIMIT = 30ULL * 1024 * 1024 * 1024; // 30GB

Result best_result;

std::string inputDir, outputFile;

size_t get_memory_usage() {
    std::ifstream statm("/proc/self/status");
    std::string line;
    size_t memory_in_kb = 0;
    while (std::getline(statm, line)) {
        if (line.find("VmRSS:") == 0) {
            memory_in_kb = std::stoull(line.substr(6)) * 1024;
            break;
        }
    }
    return memory_in_kb;
}

void run_mt(HyPar &hp, bool &valid, long long &hop) {
    hp.run();
    hp.evaluate(valid, hop);
}

void run_nc_mt(HyPar &hp, bool &valid, long long &hop) {
    hp.run_nc();
    hp.evaluate(valid, hop);
}

void next_round(std::vector<std::shared_ptr<HyPar>> &hp_mt, std::vector<std::thread> &threads, const int num_threads) {
    bool valid[num_threads]{}, bestvalid = false;
    long long hop[num_threads]{}, besthop = __LONG_LONG_MAX__;
    hp_mt[0] = std::make_shared<HyPar>(inputDir, outputFile);
    for (int i = 1; i < num_threads; ++i) {
        hp_mt[i] = std::make_shared<HyPar>(*hp_mt[0]);
    }
    for (int i = 0; i < num_threads; ++i) {
        threads[i] = std::thread(run_mt, std::ref(*hp_mt[i]), std::ref(valid[i]), std::ref(hop[i]));
    }
    size_t bestid = -1;
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
        if (bestvalid < valid[i] || hop[i] < besthop) {
            bestid = i;
            besthop = hop[i];
            bestvalid = valid[i];
        }
    }
    if (bestvalid) {
        hp_mt[bestid]->add_logic_replication_pq();
        hp_mt[bestid]->add_logic_replication_og();
        hp_mt[bestid]->evaluate(bestvalid, besthop);
        if (besthop < best_result.hop) {
            hp_mt[bestid]->printOut(best_result, besthop);
        }
    }
    for (size_t i = 0; i < hp_mt.size(); ++i) {
        hp_mt[i].reset();
    }
}

int main(int argc, char **argv) {
    std::ios::sync_with_stdio(false);
    assert(argc == 5);
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
    size_t m0 = get_memory_usage();
    std::vector<std::shared_ptr<HyPar>> hp_mt;
    std::vector<std::thread> threads;
    hp_mt.push_back(std::make_shared<HyPar>(inputDir, outputFile));
    int pin = hp_mt[0]->pin;
    hp_mt[0]->coarsen();
    size_t m1 = get_memory_usage();
    size_t hp_memory = m1 - m0;
    int num_threads = int(std::min(size_t(4), MEMORY_LIMIT /  (1 + hp_memory)));
    bool valid[num_threads]{}, bestvalid = false;
    long long hop[num_threads]{}, besthop = __LONG_LONG_MAX__;
    for (int i = 1; i < num_threads; ++i) {
        hp_mt.push_back(std::make_shared<HyPar>(*hp_mt[0]));
        size_t memory_usage = get_memory_usage();
        if (memory_usage + hp_memory >= MEMORY_LIMIT) {
            num_threads = i + 1;
            break;
        }
    }
    for (int i = 0; i < num_threads; ++i) {
        threads.emplace_back(std::thread(run_nc_mt, std::ref(*hp_mt[i]), std::ref(valid[i]), std::ref(hop[i])));
    }
    size_t bestid = -1;
    for (size_t i = 0; i < threads.size(); ++i) {
        threads[i].join();
        if (bestvalid < valid[i] || hop[i] < besthop) {
            bestid = i;
            besthop = hop[i];
            bestvalid = valid[i];
        }
    }
    if (bestvalid) {
        hp_mt[bestid]->add_logic_replication_pq();
        hp_mt[bestid]->add_logic_replication_og();
        hp_mt[bestid]->evaluate(bestvalid, besthop);
        if (besthop < best_result.hop) {
            hp_mt[bestid]->printOut(best_result, besthop);
        }
    }
    for (size_t i = 0; i < hp_mt.size(); ++i) {
        hp_mt[i].reset();
    }
    if (pin < 1e2) {
        for (int i = 0; i < 240; ++i) {
            next_round(hp_mt, threads, num_threads);
        }
    } else if (pin < 1e4) {
        for (int i = 0; i < 100; ++i) {
            next_round(hp_mt, threads, num_threads);
        }
    } else if (!bestvalid) {
        while (!bestvalid) {
            next_round(hp_mt, threads, num_threads);
            continue;
        }
    }
    std::ofstream out(outputFile);
    best_result.printOut(out);
    return 0;
}