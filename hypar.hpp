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

struct node {
    std::string name{};
    int isRep{}; // is replicated node, dont know how to handle this
    int fpga{-1}; // easy to signal the partition
    int resLoad[NUM_RES]{};
    int size{1};
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

// 并查集结构，带有节点数量统计
struct UnionFind {
    std::vector<int> parent;
    std::vector<int> rank;
    std::vector<int> size; // 用来记录每个集合的节点数量
    UnionFind() = default;
    // 初始化并查集
    UnionFind(int n) : parent(n), rank(n, 0), size(n, 1) {
        for (int i = 0; i < n; ++i) parent[i] = i;
    }


    // 查找根节点，并进行路径压缩
    int find(int x) {
        if (parent[x] != x) {
            parent[x] = find(parent[x]);
        }
        return parent[x];
    }

    // 合并两个集合
    void unite(int x, int y) {
        int rootX = find(x);
        int rootY = find(y);

        if (rootX != rootY) {
            // 按秩合并
            if (rank[rootX] > rank[rootY]) {
                parent[rootY] = rootX;
                size[rootX] += size[rootY]; // 更新 rootX 集合的节点数
            } else if (rank[rootX] < rank[rootY]) {
                parent[rootX] = rootY;
                size[rootY] += size[rootX]; // 更新 rootY 集合的节点数
            } else {
                parent[rootY] = rootX;
                size[rootX] += size[rootY]; // 更新 rootX 集合的节点数
                ++rank[rootX];
            }
        }
    }

    // 获取某个节点所在集合的节点数量
    int getSize(int x) {
        int root = find(x);
        return size[root];
    }
};





class HyPar {
private:
    int maxHop, K; // number of partitions
    std::string inputDir, outputFile;
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    UnionFind uf;    
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
    bool _fpga_chk_conn();
    bool _fpga_chk_res();
    int _max_net_gain(int tf, int u);
    int _FM_gain(int of, int tf, int u);
    int _hop_gain(int of, int tf, int u);
    int _connectivity_gain(int of, int tf, int u);
    int _gain_function(int of, int tf, int u, int sel = 0);
    void _cal_inpar_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);
    void _cal_refine_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);

public:
    HyPar() = default;
    HyPar(int numNodes);
    HyPar(std::string _inputDir, std::string _outputFile);
    HyPar(const HyPar &other) = default;
    ~HyPar() = default;

    void readInfo(std::ifstream &info);
    void readAre(std::ifstream &are);
    void readNet(std::ifstream &net);
    void readTopo(std::ifstream &topo);

    void debugUnconnectedGraph(std::ostream &out);//debug print the unconnected graph

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
    void k_way_localized_refine(int sel = 0);
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
    void evaluate(bool &valid, int &hop);
};

#endif