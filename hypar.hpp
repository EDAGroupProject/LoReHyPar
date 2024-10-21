#ifndef _HYPAR_FLAG
#define _HYPAR_FLAG

#include <fstream>
#include <iostream>
#include <queue>
#include <random>
#include <set>
#include <stack>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#define NUM_RES 8

extern std::random_device rd;
extern std::mt19937 global_rng;

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
    size_t size{};
    std::vector<int> nodes{};
};

struct fpga {
    std::string name{};
    int maxConn{}, usedConn{};
    int resCap[NUM_RES]{}, resUsed[NUM_RES]{};
    bool valid{true}; // default true, if the fpga is invalid, set it to false
    std::vector<int>neighbors{};
    std::vector<int>nodes{};

    void query_validity(){
        for (int i = 0; i < NUM_RES; ++i){
            if (resUsed[i] > resCap[i]){
                valid = false;
                break;
            }
        }
        valid = usedConn <= maxConn;
    }
};

class HyPar {
protected:
    int maxHop, K; // number of partitions
    std::string inputDir, outputFile;
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    std::vector<node> nodes;
    std::vector<net> nets;
    std::vector<fpga> fpgas;
    std::vector<std::vector<int>> fpgaMap;

public:
    HyPar(std::string _inputDir, std::string _outputFile);

    void readInfo(std::ifstream &info);
    void readAre(std::ifstream &are);
    void readNet(std::ifstream &net);
    void readTopo(std::ifstream &topo);

    void printSummary(std::ostream &out);
    void printSummary(std::ofstream &out);
    void printOut(std::ofstream &out);
};

class ParFunc : public HyPar {
private:
    int parameter_t = 2; // parameter in the calculation of ceilRes
    int parameter_l = 20; // parameter, do not evaluate nets with size more than parameter_l
    std::unordered_set<int> curNodes, delNodes;
    std::unordered_map<std::pair<int, int>, float, pair_hash> edgeRatng;
    std::stack<std::pair<int, int>> contMeme;
    std::unordered_map<int, int> netFp;
    int ceilRes[NUM_RES]{}, meanRes[NUM_RES]{};

    // basic operations
    void _contract(int u, int v);   // contract v into u
    void _uncontract(int u, int v); // uncontract v from u
    float _heavy_edge_rating(int u, int v); // rate the pair (u, v) "heavy edge"
    void _init_net_fp();
    void _detect_para_singv_net(); // detect and remove parallel nets or single vertex nets
    void _init_ceil_mean_res();
    bool _contract_eligible(int u, int v); // check if u and v are eligible to be contracted
    bool _fpga_add_try(int f, int u);
    bool _fpga_add_force(int f, int u);
    bool _fpga_remove_force(int f, int u);
    void _cal_refine_gain(int node, int f, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map);

    // test functions
    void _test_contract(std::ofstream &out);
    void _test_simple_process(std::ofstream &out);

    // Preprocessing
    // @todo: preprocessing function
    void preprocessing();

    // Coarsening
    void coarsen();
    bool coarsen_naive();
    // @todo: detect&remove parallel nets or single vertex nets

    // Initial Partitioning
    void initial_partition();
    void bfs_partition();
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
public:
    using HyPar::HyPar;
    ParFunc(const HyPar &h) : HyPar(h) {}
    void test(std::string testOutput = "test.out");
    void run();
    void evaluate();
};

#endif