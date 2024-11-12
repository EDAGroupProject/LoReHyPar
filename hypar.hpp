#ifndef _HYPAR_FLAG
#define _HYPAR_FLAG

#include <fstream>
#include <sstream>
#include <iostream>
#include <random>
#include <chrono>
#include <set>
#include <queue>
#include <stack>
#include <string>
#include <cstring>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>
#include <algorithm>
#include <cmath>
#include <climits>
#include <thread>

#define NUM_RES 8

inline std::mt19937 get_rng() {
    std::random_device rd;
    auto time_seed = std::chrono::steady_clock::now().time_since_epoch().count();
    std::seed_seq seed_seq{rd(), static_cast<unsigned>(time_seed)};
    return std::mt19937(seed_seq);
}

struct pair_hash final {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const noexcept{
        uintmax_t hash = std::hash<T1>{}(pair.first);
        hash <<= sizeof(uintmax_t) * 4; 
        hash ^= std::hash<T2>{}(pair.second);
        return std::hash<uintmax_t>{}(hash);
    }
};

struct Node {
    std::string name{};
    int rep{-1}; // -1: not a representative, >= 0: the representative
    int fpga{-1}; // easy to signal the partition
    double resLoad[NUM_RES]{};
    int size{1};
    std::set<int> nets{};
    std::unordered_map<int, bool> isSou{};
};

struct Net {
    double weight{};
    int source{};
    int size{};
    std::vector<int> nodes{};
    std::unordered_map<int, int> fpgas{}; // help to calculate the connectivity
};

struct Fpga {
    std::string name{};
    int maxConn{}, conn{};
    double resCap[NUM_RES]{}, resUsed[NUM_RES]{};
    bool resValid{true}; // update after add or remove a node
    std::unordered_set<int> nodes{};
};

struct Result {
    std::string output{};
    long long hop{LONG_LONG_MAX};
    void printOut(std::ofstream &out) {
        out << output;
    }
};

class HyPar {
private:
    int maxHop, K; // number of partitions
    std::string inputDir, outputFile;
    std::vector<Node> nodes;
    std::vector<Net> nets;
    std::vector<Fpga> fpgas;
    std::vector<std::vector<int>> fpgaMap;

    int parameter_t = 2; // parameter in the calculation of ceilRes
    int parameter_l = 20; // parameter, do not evaluate nets with size more than parameter_l
    int parameter_tau = 5; // parameter, in SCLa propagation, the tau of the neighbors get the same label
    std::unordered_set<int> existing_nodes;
    std::stack<std::pair<int, int>> contract_memo;
    double ceil_rescap[NUM_RES]{}, ceil_size{};
    double mean_res[NUM_RES]{};

    // basic operations
    void _contract(int u, int v);   // contract v into u
    void _uncontract(int u, int v, int f); // uncontract v from u
    bool _contract_eligible(int u, int v); // check if u and v are eligible to be contracted
    bool _fpga_add_try(int f, int u);
    bool _fpga_add_force(int f, int u);
    bool _fpga_remove_force(int f, int u);
    int _FM_gain(int of, int tf, int u);
    int _hop_gain(int of, int tf, int u);
    int _gain_function(int of, int tf, int u, int sel = 0);
    void _cal_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);
    void _FM_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _hop_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map);
    void _cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map);
    void _get_eligible_fpga(int u, std::unordered_set<int> &toFpga);
    void _get_eligible_fpga(int u, std::unordered_map<int, bool> &toFpga);

public:
    HyPar() = default;
    HyPar(std::string _inputDir, std::string _outputFile);

    int pin; // number of pins

    void readInfo(std::ifstream &info, std::unordered_map<std::string, int> &fpga2id);
    void readAre(std::ifstream &are, std::unordered_map<std::string, int> &node2id);
    void readNet(std::ifstream &net, std::unordered_map<std::string, int> &node2id);
    void readTopo(std::ifstream &topo, std::unordered_map<std::string, int> &fpga2id);

    void printOut();
    void printOut(std::ofstream &out);
    void printOut(Result &res, long long hop);

    // Coarsening
    void coarsen();
    bool coarsen_by_nets();

    // Initial Partitioning
    void initial_partition();
    void fast_initial_partition();
    void SCLa_propagation();
    void greedy_hypergraph_growth(int sel);
    void activate_max_hop_nodes(int sel);

    // Refinement
    void refine();
    void fast_refine();
    void only_fast_refine();
    void k_way_localized_refine(int sel);
    void fast_k_way_localized_refine(int num, int sel);
    void only_fast_k_way_localized_refine(int num, int sel);
    void add_logic_replication_og();
    void add_logic_replication_pq();
    void add_logic_replication_rd();

    void run();
    void run_nc();
    void evaluate(bool &valid, long long &hop);
};

#endif
