#include <hypar.hpp>
#include <cassert>
#include <sstream>
#include <iomanip>

LoReHyPar::LoReHyPar(const std::string &path){
    std::ifstream info(path + "design.info"), are(path + "design.are"), net(path + "design.net"), topo(path + "design.topo");
    assert(info && are && net && topo);
    readInfo(info);
    readAre(are);
    readNet(net);
    readTopo(topo);
}

void LoReHyPar::readInfo(std::ifstream &info){
    std::string name;
    while(info >> name){
        fpga2id[name] = fpgas.size();
        fpgas.emplace_back(fpga{name});
        info >> fpgas.back().maxConn;
        for (int i = 0; i < NUM_RES; i++){
            info >> fpgas.back().resCap[i];
        }
    }
}

void LoReHyPar::readAre(std::ifstream &are){
    std::string name;
    while(are >> name){
        node2id[name] = nodes.size();
        nodes.emplace_back(node{name});
        for (int i = 0; i < NUM_RES; i++){
            are >> nodes.back().resLoad[i];
        }
    }
}

void LoReHyPar::readNet(std::ifstream &net){
    std::string line;
    std::string name;
    int id;
    while (std::getline(net, line)){
        std::istringstream iss(line);
        nets.emplace_back();
        iss >> name;
        id = node2id[name];
        nets.back().nodes.emplace_back(id);
        nets.back().source = id;
        nodes[id].nets.emplace_back(nets.size() - 1);
        nodes[id].isSou[nets.size() - 1] = true;
        iss >> nets.back().weight;
        while (iss >> name) {
            id = node2id[name];
            nets.back().nodes.emplace_back(id);
            nodes[id].nets.emplace_back(nets.size() - 1);
        }
        nets.back().size = nets.back().nodes.size();
    }
}

void LoReHyPar::readTopo(std::ifstream &topo){
    topo >> maxHop;
    int n = fpgas.size();
    fpgaMap.assign(n, std::vector<int>(n, maxHop + 1)); // maxHop + 1 means no connection
    for (int i = 0; i < n; i++){
        fpgaMap[i][i] = 0; // distance to itself is 0, neccessary for floyd-warshall
    }
    std::string name1, name2;
    int id1, id2;
    while (topo >> name1 >> name2){
        id1 = fpga2id[name1];
        id2 = fpga2id[name2];
        fpgas[id1].neighbors.emplace_back(id2);
        fpgas[id2].neighbors.emplace_back(id1);
        fpgaMap[id1][id2] = fpgaMap[id2][id1] = 1;
    }
    for (int k = 0; k < n; k++){
        for (int i = 0; i < n; i++){
            for (int j = 0; j < n; j++){
                if (fpgaMap[i][j] > fpgaMap[i][k] + fpgaMap[k][j]){
                    fpgaMap[i][j] = fpgaMap[i][k] + fpgaMap[k][j];
                }
            }
        }
    }
    for (int i = 0; i < n; i++){
            for (int j = 0; j < n; j++){
                if(fpgaMap[i][j] > maxHop){
                    fpgaMap[i][j] = -1;
                }
            }
        }
}

void LoReHyPar::printSummary(std::ostream &out){
    out << "Nodes: " << nodes.size() << std::endl;
    for (size_t i = 0; i < nodes.size(); i++){
        auto &node = nodes[i];
        out << "  node " << i << ": " << node.name << "-" << std::to_string(node.isRep) << "\n    resLoad: ";
        for (int i = 0; i < NUM_RES; i++){
            out << node.resLoad[i] << " ";
        }
        out << "\n    nets: ";
        for (auto net : node.nets){
            out << net << " ";
        }
        out << std::endl;
    }
    out << "Nets: " << nets.size() << std::endl;
    for (size_t i = 0; i < nets.size(); i++){
        auto &net = nets[i];
        out << "  net " << i << ": \n    weight: ";
        out << net.weight << "\n    size: ";
        out << net.size << "\n    source: ";
        out << net.source << "\n    nodes: ";
        for (int node : net.nodes){
            out << node << " ";
        }
        out << std::endl;
    }
    out << "FPGAs: " << fpgas.size() << std::endl;
    for (size_t i = 0; i < fpgas.size(); i++){
        auto &fpga = fpgas[i];
        out << "  fpga " << i << ": " << fpga.name << "\n    maxConn: " << fpga.maxConn << "\n    resCap: ";
        for (int i = 0; i < NUM_RES; i++){
            out << fpga.resCap[i] << " ";
        }
        out << "\n    neighbors: ";
        for (auto neighbor : fpga.neighbors){
            out << neighbor << " ";
        }
        out << "\n    nodes: ";
        for (auto node : fpga.nodes){
            out << node << " ";
        }
        out << std::endl;
    }
    out << "MaxHop: " << maxHop << std::endl;
    out << "FPGA Map:\n";
    for (auto &row : fpgaMap){
        out << "  ";
        for (auto &cell : row){
            out << std::setw(3) << std::setfill(' ') << cell << " ";
        }
        out << std::endl;
    }
}

void LoReHyPar::printOut(std::ofstream &out){
    for (auto &fpga: fpgas){
        out << fpga.name << ": ";
        for (auto node : fpga.nodes){
            if(nodes[node].isRep){
                out << nodes[node].name << "* ";
            }else{
                out << nodes[node].name << " ";
            }
        }
        out << std::endl;
    }
}

void LoReHyPar::test_contract(){
    int u = 0;
    for (size_t v = 1; v < nodes.size(); ++v){
        _contract(u, v);
        std::cout << "Contract " << u << " " << v << std::endl;
        printSummary(std::cout);
    }
    while (!cont_meme.empty()){
        auto [u, v] = cont_meme.top();
        _uncontract(u, v);
        std::cout << "Uncontract " << u << " " << v << std::endl;
        printSummary(std::cout);
    }
}