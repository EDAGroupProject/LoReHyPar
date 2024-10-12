#ifndef _PARFUNCS_FLAG
#define _PARFUNCS_FLAG

#include <hypar.hpp>
#include <stack>
#include <unordered_set>

// We todo to gradually enrich the functions 
// of each step in the future.

class LoReHyPar;

class ParFunc {
    friend class LoReHyPar;

    int k; // number of partitions
    int parameter_t = 2; // parameter in the calculation of ceilRes
    int parameter_l = 20; // parameter, do not evaluate nets with size more than parameter_l
    std::unordered_set<int> curNodes, delNodes;
    std::stack<std::pair<int, int>> contMeme;
    std::unordered_map<int, int> netFp;
    int ceilRes[NUM_RES]{}, meanRes[NUM_RES]{};

    ParFunc(LoReHyPar &hp): k(hp.np) {}

    // basic operations
    void _contract(LoReHyPar &hp, int u, int v);   // contract v into u
    void _uncontract(LoReHyPar &hp, int u, int v); // uncontract v from u
    inline float _heavy_edge_rating(LoReHyPar &hp, int u, int v); // rate the pair (u, v) "heavy edge"
    void _init_net_fp(LoReHyPar &hp);
    void _detect_para_singv_net(LoReHyPar &hp); // detect and remove parallel nets or single vertex nets
    void _init_ceil_mean_res(LoReHyPar &hp);
    bool _contract_eligible(LoReHyPar &hp, int u, int v); // check if u and v are eligible to be contracted
    bool _fpga_add_res(LoReHyPar &hp, int f, int u);
    bool _fpga_cal_conn(LoReHyPar &hp, int f); // @todo: now full update, maybe we can incrementally update the connection

    // test functions
    void _test_contract(LoReHyPar &hp);

    // Preprocessing
    // @todo: preprocessing function
    void preprocessing(LoReHyPar &hp);

    // Coarsening
    bool coarsen_naive(LoReHyPar &hp);
    // @todo: detect&remove parallel nets or single vertex nets
    void coarsen(LoReHyPar &hp);

    // Initial Partitioning
    void bfs_partition(LoReHyPar &hp);
    // @todo: other partitioning methods to enrich the portfolio
    void initial_partition(LoReHyPar &hp);
    
    // Map FGPA
    // @todo: mapping function
    void map_fpga(LoReHyPar &hp);

    // Refinement
    // @todo: refinement function
    void refine(LoReHyPar &hp);

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
};

#endif