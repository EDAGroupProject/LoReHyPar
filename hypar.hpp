#ifndef hypar_hpp
#define hypar_hpp

#include <vector>
#include <unordered_map>
#include <string>
#include <iostream>
#include <fstream>

#define NUM_RES 8

struct node{
    std::string name;
    bool isRep{}; // is replicated node, dont know how to handle this
    int resLoad[NUM_RES]{};
    std::vector<int> nets;
};

struct net{
    int weight;
    std::vector<int> nodes; // first node is the source
};

struct fpga{
    std::string name;
    int maxConn;
    int resCap[NUM_RES]{};
    std::vector<int>neighbors;
    std::vector<int>nodes;
};

class LoReHyPar{
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    std::vector<node> nodes;
    std::vector<net> nets;
    std::vector<fpga> fpgas;
    int maxHop;
    std::vector<std::vector<int>> fpgaMap;
public:
    LoReHyPar(const std::string &path = "");

    void readInfo(std::ifstream &info);
    void readAre(std::ifstream &are);
    void readNet(std::ifstream &net);
    void readTopo(std::ifstream &topo);

    void printSummary(std::ostream &out);
    void printOut(std::ofstream &out);
};

#endif