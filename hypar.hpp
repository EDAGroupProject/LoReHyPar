#ifndef _HYPAR_FLAG
#define _HYPAR_FLAG

#include <fstream>
#include <iostream>
#include <random>
#include <chrono>
#include <set>
#include <queue>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <cassert>
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

class HyPar {
private:
    int maxHop, K; // number of partitions
    std::string inputDir, outputFile;
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    std::vector<Node> nodes;
    std::vector<Net> nets;
    std::vector<Fpga> fpgas;
    std::vector<std::vector<int>> fpgaMap;

    int parameter_t = 2; // parameter in the calculation of ceilRes
    int parameter_l = 20; // parameter, do not evaluate nets with size more than parameter_l
    int parameter_tau = 5; // parameter, in SCLa propagation, the tau of the neighbors get the same label
    std::unordered_set<int> existing_nodes;
    std::unordered_map<int, int> node2community;
    std::stack<std::pair<int, int>> contract_memo;
    double ceil_rescap[NUM_RES]{}, ceil_size{};
    double mean_res[NUM_RES]{};

    // basic operations
    void _contract(int u, int v);   // contract v into u
    void _uncontract(int u, int v); // uncontract v from u
    void _uncontract(int u, int v, int f);  // uncontract v from u
    bool _contract_eligible(int u, int v); // check if u and v are eligible to be contracted
    bool _fpga_add_try(int f, int u);
    bool _fpga_add_force(int f, int u);
    bool _fpga_remove_force(int f, int u);
    long long _fpga_cal_conn();
    bool _fpga_chk_conn();
    bool _fpga_chk_res();
    int _max_net_gain(int tf, int u);
    int _FM_gain(int of, int tf, int u);
    int _hop_gain(int of, int tf, int u);
    int _connectivity_gain(int of, int tf, int u);
    int _gain_function(int of, int tf, int u, int sel = 0);
    void _cal_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);
    void _max_net_gain(std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _FM_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _hop_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _connectivity_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map);
    void _cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map);
    void _get_eligible_fpga(int u, std::unordered_set<int> &toFpga);
    void _get_eligible_fpga(int u, std::unordered_map<int, bool> &toFpga);
    bool _chk_legel_put();
    bool _chk_net_fpgas();

public:
    HyPar() = default;
    HyPar(std::string _inputDir, std::string _outputFile);

    int N; // number of nodes

    void readInfo(std::ifstream &info);
    void readAre(std::ifstream &are);
    void readNet(std::ifstream &net);
    void readTopo(std::ifstream &topo);

    void printOut();
    void printOut(std::ostream &out);
    void printOut(std::ofstream &out);

    // Preprocessing
    void preprocess();
    void community_detect();

    // Coarsening
    void coarsen();
    void fast_coarsen();
    bool coarsen_by_nets();
    bool coarsen_by_nets_in_community();

    // Initial Partitioning
    void initial_partition();
    void fast_initial_partition();
    void bfs_partition();
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

    void run();
    void run_before_coarsen();
    void run_after_coarsen(bool &valid, long long &hop);
    void evaluate_summary(std::ostream &out);
    void evaluate_summary(bool &valid, long long &hop, std::ostream &out);
    void evaluate(bool &valid, long long &hop);
};

#endif
