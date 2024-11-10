#include <hypar.hpp>
#include <unistd.h>
#include <memory>
#include <sys/resource.h>

const size_t MEMORY_LIMIT = 31.9 * 1024 * 1024 * 1024; // 31.9GB

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

void limit_memory_usage() {
    struct rlimit rl;
    rl.rlim_cur = 32L * 1024 * 1024 * 1024; // 32 GB
    rl.rlim_max = 32L * 1024 * 1024 * 1024; // 32 GB
    if (setrlimit(RLIMIT_AS, &rl) != 0) {
        std::cerr << "Failed to set memory limit." << std::endl;
    }
}

bool manage_memory_and_threads(HyPar &hp, size_t hp_memory, int num_threads = 4) {
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
        hp_mt[bestid]->add_logic_replication();
        hp_mt[bestid]->printOut();
        return true;
    }
    return false;
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
    size_t m0 = get_memory_usage();
    HyPar hp(inputDir, outputFile);
    hp.run_before_coarsen();
    size_t m1 = get_memory_usage();
    size_t memory_usage = std::max(m1 - m0, size_t(1024 * 1024 * 1024));
    std::cout << "Memory usage: " << memory_usage << std::endl;
    const int num_threads = std::min(4, static_cast<int>(MEMORY_LIMIT /  memory_usage - 1));
    std::cout << "num_threads: " << num_threads << std::endl;
    if (num_threads >= 1) {
        while (!manage_memory_and_threads(hp, memory_usage, num_threads)) {
            std::cout << "Run with " << num_threads << " threads." << std::endl;
        } 
    } else {
        std::cout << "Run with 1 thread." << std::endl;
        bool valid = false;
        long long hop = 0;
        hp.run_after_coarsen(valid, hop);
        while (!valid) {
            std::cout << "Run with 1 thread." << std::endl;
            hp.reread();
            hp.run(valid, hop);
        }
        hp.printOut();
    }
    return 0;
}