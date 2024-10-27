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

struct node {
    std::string name{};
    int isRep{}; // is replicated node, dont know how to handle this
    int fpga{-1}; // easy to signal the partition
    int resLoad[NUM_RES]{};
    std::vector<int> reps{};
    std::set<int> nets{};
    std::unordered_map<int, bool> isSou{};
};

struct net {
    int weight{};
    int source{};
    int size{};
    std::vector<int> nodes{};
    std::unordered_map<int, int> fpgas{}; // help to calculate the connectivity
    // when kvp.second == 0, it should be erased
};

struct fpga {
    std::string name{};
    int maxConn{}, conn{};
    int resCap[NUM_RES]{}, resUsed[NUM_RES]{};
    bool resValid{true}; // update after add or remove a node
    std::vector<int> nodes{};
};

class HyPar {
private:
    int maxHop, K; // number of partitions
    std::string inputDir, outputFile;
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    std::vector<node> nodes;
    int resLoadAll[NUM_RES]{};
    std::vector<net> nets;
    std::vector<fpga> fpgas;
    std::vector<std::vector<int>> fpgaMap;

    int parameter_t = 2; // parameter in the calculation of ceilRes
    int parameter_l = 20; // parameter, do not evaluate nets with size more than parameter_l
    int parameter_tau = 5; // parameter, in SCLa propagation, the tau of the neighbors get the same label
    std::unordered_set<int> existing_nodes, deleted_nodes;
    std::unordered_map<int, int> node2community;
    std::vector<std::unordered_set<int>> communities;
    std::stack<std::pair<int, int>> contract_memo;
    // std::unordered_map<int, int> netFp;
    int ceil_rescap[NUM_RES]{};

    // basic operations
    void _contract(int u, int v);   // contract v into u
    void _uncontract(int u, int v); // uncontract v from u
    float _heavy_edge_rating(int u, int v); // rate the pair (u, v) "heavy edge"
    // void _init_net_fp();
    // void _detect_para_singv_net(); // detect and remove parallel nets or single vertex nets
    void _init_ceil_res();
    bool _contract_eligible(int u, int v); // check if u and v are eligible to be contracted
    bool _fpga_add_try(int f, int u);
    bool _fpga_add_try_nochange(int f, int u);
    bool _fpga_add_force(int f, int u);
    bool _fpga_remove_force(int f, int u);
    void _fpga_cal_conn();
    int _max_net_gain(int tf, int u);
    int _FM_gain(int of, int tf, int u);
    int _connectivity_gain(int of, int tf, int u);
    int _gain_function(int of, int tf, int u, int sel = 0);
    void _cal_refine_gain(int node, int f, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);

public:
    HyPar() = default;
    HyPar(std::string _inputDir, std::string _outputFile);

    void readInfo(std::ifstream &info);
    void readAre(std::ifstream &are);
    void readNet(std::ifstream &net);
    void readTopo(std::ifstream &topo);

    void printSummary(std::ostream &out);
    void printSummary(std::ofstream &out);
    void printOut(std::ofstream &out);

    // Preprocessing
    // @todo: preprocessing function
    void preprocess();
    void pin_sparsify();
    void community_detect();

    // Coarsening
    void coarsen();
    bool _coarsen_naive();
    void coarsen_naive();
    bool coarsen_in_community(int community);
    // @todo: detect&remove parallel nets or single vertex nets

    // Initial Partitioning
    void initial_partition();
    void bfs_partition();
    void SCLa_propagation();
    void greedy_hypergraph_growth(int sel = 0);
    // @todo: other partitioning methods to enrich the portfolio

    // Map FGPA
    // @todo: mapping function
    void map_fpga();

    // Refinement
    void refine();
    void refine_naive();
    void k_way_localized_refine();
    void random_validity_refine();
        
    // Replication
    // @todo: the implementation of the replication

    // Multi-thread
    // @todo: i have include the boost library, in the future, we can use it to implement multi-threading
    
    // Flow-cutter
    // @todo: maybe use the flow-cutter to refine the partition

    // Memetic Algorithm
    // @todo: implement the memetic algorithm to the whole framework

    // Incremental Update or Data Cache
    // @warning: this is a very important part, we should implement this in the future

    void run();
    void evaluate_summary(std::ostream &out);
    void evaluate(bool &valid, int &hop);
};

#endif