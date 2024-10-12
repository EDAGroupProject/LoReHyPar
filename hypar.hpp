#ifndef _HYPAR_FLAG
#define _HYPAR_FLAG

#include <vector>
#include <unordered_map>
#include <set>
#include <string>
#include <iostream>
#include <fstream>

#define NUM_RES 8

struct pair_hash {
    template <class T1, class T2>
    std::size_t operator() (const std::pair<T1, T2> &pair) const {
        return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
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
    std::vector<int> nodes;
};

struct fpga {
    std::string name{};
    int maxConn{}, usedConn{};
    int resCap[NUM_RES]{}, resUsed[NUM_RES]{};
    std::vector<int>neighbors{};
    std::vector<int>nodes{};
};

class ParFunc;

class LoReHyPar {
    std::string path; // path/to/input/dir/ with the last '/'
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    std::vector<node> nodes;
    std::vector<net> nets;
    std::vector<fpga> fpgas;
    int maxHop;
    std::vector<std::vector<int>> fpgaMap;
    int np; // number of partitions

    friend class ParFunc;

public:
    LoReHyPar(const std::string &path = "");

    void readInfo(std::ifstream &info);
    void readAre(std::ifstream &are);
    void readNet(std::ifstream &net);
    void readTopo(std::ifstream &topo);

    void printSummary(std::ostream &out);
    void printOut(std::ofstream &out);

    void test();
    void run();
};

#endif