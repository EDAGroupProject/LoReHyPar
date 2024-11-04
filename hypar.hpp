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
    int isRep{}; // is replicated node, dont know how to handle this
    int fpga{-1}; // easy to signal the partition
    double resLoad[NUM_RES]{};
    int size{1};
    int fp{};
    std::vector<int> reps{};
    std::unordered_set<int> nets{};
    std::unordered_map<int, bool> isSou{};
};

struct Net {
    double weight{};
    int source{};
    int size{};
    std::vector<int> nodes{};
    std::unordered_map<int, int> fpgas{}; // help to calculate the connectivity
    // when kvp.second == 0, it should be erased
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
    std::unordered_set<int> existing_nodes, deleted_nodes;
    std::vector<std::unordered_set<int>> communities;
    std::unordered_map<int, int> node2community;
    std::stack<std::pair<int, int>> contract_memo;
    // std::unordered_map<int, int> netFp;
    double ceil_rescap[NUM_RES]{}, ceil_size{};
    double mean_res[NUM_RES]{};

    // basic operations
    void _contract(int u, int v);   // contract v into u
    void _uncontract(int u, int v); // uncontract v from u
    void _uncontract(int u, int v, int f);  // uncontract v from u
    float _heavy_edge_rating(int u, int v); // rate the pair (u, v) "heavy edge"
    float _heavy_edge_rating(int u, int v, std::unordered_map<std::pair<int, int>, int, pair_hash> &rating); // rate the pair (u, v) "heavy edge"
    // void _init_net_fp();
    // void _detect_para_singv_net(); // detect and remove parallel nets or single vertex nets
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
    void _cal_inpar_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);
    void _cal_refine_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);
    void _max_net_gain(std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _FM_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _hop_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _connectivity_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map);
    void _cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map);
    void _get_eligible_fpga(int u, std::unordered_set<int> &toFpga);
    void _get_eligible_fpga(int u, std::unordered_map<int, bool> &toFpga);
    bool _chk_legel_put();


    // fastdebug
    void savePreprocessResults(const std::string &filename);
    void loadPreprocessResults(const std::string &filename);
    void savePreprocessResultsAsText(const std::string &filename);
    void loadPreprocessResultsAsText(const std::string &filename);
    void saveCoarsenResults(const std::string &filename);
    void loadCoarsenResults(const std::string &filename);
    void saveCoarsenResultsAsText(const std::string &filename);
    void loadCoarsenResultsAsText(const std::string &filename);
    void savePartitionResults(const std::string &filename);
    void loadPartitionResults(const std::string &filename);
    void savePartitionResultsAsText(const std::string &filename);
    void loadPartitionResultsAsText(const std::string &filename);
    void saveRefineResults(const std::string &filename);
    void loadRefineResults(const std::string &filename);
    void saveRefineResultsAsText(const std::string &filename);
    void loadRefineResultsAsText(const std::string &filename);

public:
    HyPar() = default;
    HyPar(std::string _inputDir, std::string _outputFile);
    HyPar(const HyPar &other) = default;
    ~HyPar() = default;

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
    void pin_sparsify_in_community(int community, int c_min, int c_max);
    void fast_pin_sparsify();
    void fast_pin_sparsify_in_community(int community, int c_min, int c_max);
    int community_detect();
    void contract_in_community(int community);
    void contract_in_community(const std::unordered_set<int> &community, std::unordered_set<int> &active_nodes);
    void recursive_community_contract();
    void preprocess_for_next_round();

    // Coarsening
    void coarsen();
    void fast_coarsen();
    bool _coarsen_naive();
    void coarsen_naive();
    bool coarsen_in_community(int community);
    bool fast_coarsen_in_community(int community);
    bool naive_coarsen_in_community(int community);
    bool coarsen_by_nets();
    // @todo: detect&remove parallel nets or single vertex nets

    // Initial Partitioning
    void initial_partition();
    void fast_initial_partition();
    void bfs_partition();
    void SCLa_propagation();
    void greedy_hypergraph_growth(int sel);
    // @todo: other partitioning methods to enrich the portfolio

    // Refinement
    void refine();
    void fast_refine();
    void only_fast_refine();
    void refine_naive();
    void k_way_localized_refine(int sel);
    void fast_k_way_localized_refine(int num, int sel);
    void only_fast_k_way_localized_refine(int num, int sel);
    bool refine_max_connectivity();
    bool refine_res_validity(int sel);
    bool refine_max_hop();
    void refine_random_one_node(int sel);
    void force_connectivity_refine();
    bool force_validity_refine(int sel = 0);
        
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
    void evaluate(bool &valid, long long &hop);
};

#endif
