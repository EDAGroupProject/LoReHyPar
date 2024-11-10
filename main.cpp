#include <hypar.hpp>
#include <unistd.h>
#include <memory>
#include <sys/resource.h>

const size_t MEMORY_LIMIT = 31.9 * 1024 * 1024 * 1024; // 31.9GB

Result best_result;

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
    hp.run_after_coarsen(valid, hop);
}

void manage_memory_and_threads(HyPar &hp, size_t hp_memory, int num_threads = 4) {
    std::vector<std::shared_ptr<HyPar>> hp_mt;
    std::vector<std::thread> threads;
    bool valid[num_threads]{}, bestvalid = false;
    long long hop[num_threads]{}, besthop = __LONG_LONG_MAX__;
    for (int i = 0; i < num_threads; ++i) {
        auto hp_instance = std::make_shared<HyPar>(hp);
        hp_mt.push_back(hp_instance);
        threads.emplace_back(run_mt, std::ref(*hp_mt.back()), std::ref(valid[i]), std::ref(hop[i]));
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        size_t memory_usage = get_memory_usage();
        std::cout << "Memory usage: " << memory_usage << std::endl;
        if (memory_usage + hp_memory >= MEMORY_LIMIT) {
            std::cout << "Memory limit reached, stop pushing hp" << std::endl;
            break;
        }
    }
    size_t bestid = -1;
    for (size_t i = 0; i < threads.size(); ++i) {
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
        for (size_t i = 0; i < hp_mt.size(); ++i) {
            if (i != bestid) {
                hp_mt[i].reset();
            }
        }
        hp_mt[bestid]->add_logic_replication(besthop);
        if (besthop < best_result.hop) {
            hp_mt[bestid]->printOut(best_result, besthop);
        }
    }
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
    size_t m0 = get_memory_usage();
    HyPar hp(inputDir, outputFile);
    hp.run_before_coarsen();
    size_t m1 = get_memory_usage();
    size_t memory_usage = m1 - m0;
    std::cout << "Memory usage: " << memory_usage << std::endl;
    const int num_threads = int(std::min(size_t(4), MEMORY_LIMIT /  (1 + memory_usage) - 1));
    std::cout << "num_threads: " << num_threads << std::endl;
    if (num_threads >= 1) {
        std::cout << "Run with " << num_threads << " threads." << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        manage_memory_and_threads(hp, memory_usage, num_threads);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "Time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;
        const int round_num = int(std::min(long(100), 55 * 60 / (1 + std::chrono::duration_cast<std::chrono::seconds>(end - start).count())));
        std::cout << "Round number: " << round_num << std::endl;
        for (int i = 1; i < round_num; ++i) {
            manage_memory_and_threads(hp, memory_usage, num_threads);
        }
        std::ofstream out(outputFile);
        best_result.printOut(out);
    } else {
        std::cout << "Run with 1 thread." << std::endl;
        bool valid = false;
        long long hop = 0;
        hp.run_after_coarsen(valid, hop);
        if (valid) {
            hp.add_logic_replication(hop);
            hp.printOut();
            return 0;
        }
        else
            while (!valid) {
                hp.reread();
                hp.run(valid, hop);
                if (valid) {
                    hp.add_logic_replication(hop);
                    hp.printOut();
                    return 0;
                }
            }
    }
    return 0;
}