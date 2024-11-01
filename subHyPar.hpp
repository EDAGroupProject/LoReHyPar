#ifndef _SUBHYPAR_HPP_
#define _SUBHYPAR_HPP_

#include <hypar.hpp>
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
struct pair_hash;
struct Node;
struct Net;
struct Fpga;

class HyPar;

class subHyPar {
private:
    friend class HyPar;
    int maxHop, K; // number of partitions
    std::string inputDir, outputFile;
    std::unordered_map<int, Node> nodes;
    int resLoadAll[NUM_RES]{};
    std::unordered_map<int, Net> nets;
    std::vector<Fpga> fpgas;
    std::vector<std::vector<int>> fpgaMap;

    int parameter_t = 2; // parameter in the calculation of ceilRes
    int parameter_l = 20; // parameter, do not evaluate nets with size more than parameter_l
    int parameter_tau = 5; // parameter, in SCLa propagation, the tau of the neighbors get the same label
    std::unordered_set<int> existing_nodes, deleted_nodes;
    std::stack<std::pair<int, int>> contract_memo;
    int ceil_rescap[NUM_RES]{}, ceil_size{};

    void _contract(int u, int v);   // contract v into u
    void _uncontract(int u, int v); // uncontract v from u
    void _uncontract(int u, int v, int f);  // uncontract v from u
    float _heavy_edge_rating(int u, int v); // rate the pair (u, v) "heavy edge"
    float _heavy_edge_rating(int u, int v, std::unordered_map<std::pair<int, int>, int, pair_hash> &rating); // rate the pair (u, v) "heavy edge"
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
    void _max_net_gain(std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _FM_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _hop_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _connectivity_gain(int of, std::unordered_set<int> &tf, int u, std::unordered_map<int, int> &gain_map);
    void _gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map);
    void _cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map);

public:
    subHyPar() = default;
    subHyPar(const HyPar &hp, const std::unordered_set<int> &hp_nodes);
    subHyPar(const subHyPar &other) = default;
    ~subHyPar() = default;

    bool _coarsen_naive();
    void coarsen_naive();

    void initial_partition();
    void bfs_partition();
    void SCLa_propagation();
    void greedy_hypergraph_growth(int sel);

    void refine();
    void k_way_localized_refine(int sel);
    bool force_validity_refine(int sel);

    void run();
    void evaluate(bool &valid, long long &hop);
};

#endif