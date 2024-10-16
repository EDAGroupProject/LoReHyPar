#include <hypar.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <sstream>


std::random_device rd;
std::mt19937 global_rng(rd());

HyPar::HyPar(std::string _inputDir, std::string _outputFile) : inputDir(_inputDir), outputFile(_outputFile) {
    std::ifstream info(inputDir + "/design.info"), are(inputDir + "/design.are"), net(inputDir + "/design.net"), topo(inputDir + "/design.topo");
    assert(info && are && net && topo);
    readInfo(info);
    readAre(are);
    readNet(net);
    readTopo(topo);
}

void HyPar::readInfo(std::ifstream &info){
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

void HyPar::readAre(std::ifstream &are){
    std::string name;
    while(are >> name){
        node2id[name] = nodes.size();
        nodes.emplace_back(node{name});
        for (int i = 0; i < NUM_RES; i++){
            are >> nodes.back().resLoad[i];
        }
    }
}

void HyPar::readNet(std::ifstream &net){
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
        nodes[id].nets.insert(nets.size() - 1);
        nodes[id].isSou[nets.size() - 1] = true;
        iss >> nets.back().weight;
        while (iss >> name) {
            id = node2id[name];
            nets.back().nodes.emplace_back(id);
            nodes[id].nets.insert(nets.size() - 1);
        }
        nets.back().size = nets.back().nodes.size();
    }
}

void HyPar::readTopo(std::ifstream &topo){
    topo >> maxHop;
    K = fpgas.size();
    fpgaMap.assign(K, std::vector<int>(K, maxHop + 1)); // maxHop + 1 means no connection
    for (int i = 0; i < K; i++){
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
    for (int k = 0; k < K; k++){
        for (int i = 0; i < K; i++){
            for (int j = 0; j < K; j++){
                if (fpgaMap[i][j] > fpgaMap[i][k] + fpgaMap[k][j]){
                    fpgaMap[i][j] = fpgaMap[i][k] + fpgaMap[k][j];
                }
            }
        }
    }
    for (int i = 0; i < K; i++){
            for (int j = 0; j < K; j++){
                if(fpgaMap[i][j] > maxHop){
                    fpgaMap[i][j] = -1;
                }
            }
        }
}

void HyPar::printSummary(std::ostream &out){
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
        out << "\n    reps: ";
        for (auto rep : node.reps){
            out << rep << " ";
        }
        out << "\n    fpga: " << node.fpga << std::endl;
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

void HyPar::printSummary(std::ofstream &out){
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
        out << "\n    reps: ";
        for (auto rep : node.reps){
            out << rep << " ";
        }
        out << "\n    fpga: " << node.fpga << std::endl;
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

void HyPar::printOut(std::ofstream &out){
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

void ParFunc::_contract(int u, int v){
    contMeme.push({u, v});
    curNodes.erase(v);
    delNodes.insert(v);
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] += nodes[v].resLoad[i];
    }
    for (int net : nodes[v].nets){
        if (nets[net].size == 1){ // v is the only node in the net, relink (u, net)
            nets[net].nodes[0] = u;
            nets[net].source = u;
            nodes[u].nets.insert(net);
            nodes[u].isSou[net] = true;
            netFp[net] += u * u - v * v; // incrementally update the fingerprint
        }else{
            auto vit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), v);
            if (nodes[v].isSou[net]){
                nets[net].source = u;
                nodes[u].isSou[net] = true;
            }
            if (!nodes[u].nets.count(net)){ // u is not in the net, relink (u, net)
                *vit = u;
                nodes[u].nets.insert(net);
                netFp[net] += u * u - v * v; // incrementally update the fingerprint
            }
            else{ // u is in the net, delete v
                *vit = nets[net].nodes[--nets[net].size];
                nets[net].nodes[nets[net].size] = v;
                netFp[net] -= v * v; // incrementally update the fingerprint
            }
        }
    }
}

void ParFunc::_uncontract(int u, int v){
    assert(contMeme.top() == std::make_pair(u, v));
    contMeme.pop();
    curNodes.insert(v);
    delNodes.erase(v);
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] -= nodes[v].resLoad[i];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    for (int net : secNets){
        if (nodes[v].isSou[net]){
            nets[net].source = v;
            nodes[u].isSou[net] = false;
        }
        if (nets[net].nodes.size() == nets[net].size || nets[net].nodes[nets[net].size] != v){ // v is relinked
            auto uit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u);
            *uit = v;
            nodes[u].nets.erase(net);
        }else{ // v is deleted
            ++nets[net].size;
        }
    }
}

// @warning: the following functions haven't been tested by Haichuan liu!
// @todo: check the correctness of the following functions

float ParFunc::_heavy_edge_rating(int u, int v){
    if (edgeRatng.count({u, v})){
        return edgeRatng[{u, v}];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    float rating = 0;
    for (int net : secNets){
        if (nets[net].size > static_cast<size_t>(parameter_l)){
            continue;
        }
        rating += (float) nets[net].weight / (nets[net].size - 1); // nets[net].size >= 2
    }
    return rating;
}

void ParFunc::_init_net_fp(){
    for (size_t i = 0; i < nets.size(); ++i){
        netFp[i] = 0;
        for (int node : nets[i].nodes){
            netFp[i] += node * node;
        }
    }
}

void ParFunc::_detect_para_singv_net(){
    // for a deleted net, do we need to record it and plan to recover it?
    // will deleting a net fail the uncontract operation?
    // maybe we need to record the order of deletion and the level
    for (size_t i = 0; i < nets.size(); ++i){
        if (nets[i].size == 1){
            netFp[i] = 0; // delete single vertex net
        }
    }
    std::vector<int> indices(nets.size());
    std::iota(indices.begin(), indices.end(), 0); 
    std::sort(indices.begin(), indices.end(), [&](int i, int j){return netFp[i] < netFp[j];});
    bool paraFlag = false;
    int paraNet = -1, fp = -1;
    for (int i : indices){
        if (netFp[i] == 0){
            continue;
        }
        if (!paraFlag){
            fp = netFp[i];
            paraNet = i;
        } else if (netFp[i] == fp){
            paraFlag = true;
            nets[paraNet].weight += nets[i].weight; // merge parallel nets
            netFp[i] = 0;
        } else{
            paraFlag = false;
            fp = -1;
            paraNet = -1;
        }
    }
    // finally, do we really need this function?
}

void ParFunc::_init_ceil_mean_res(){
    memset(ceilRes, 0, sizeof(ceilRes));
    memset(meanRes, 0, sizeof(meanRes));
    for (int i = 0; i < NUM_RES; ++i){
        for (auto &fpga : fpgas){
            ceilRes[i] = std::max(ceilRes[i], static_cast<int>(std::ceil((float) fpga.resCap[i] / K * parameter_t)));
            meanRes[i] += fpga.resCap[i];
        }
        meanRes[i] = static_cast<int>(std::ceil(meanRes[i] / K ));
    }
}

bool ParFunc::_contract_eligible(int u, int v){
    for (int i = 0; i < NUM_RES; ++i){
        if (nodes[u].resLoad[i] + nodes[v].resLoad[i] > ceilRes[i]){
            return false;
        }
    }
    return true;
}

bool ParFunc::_fpga_add_res(int f, int u){
    int tmp[NUM_RES];
    std::copy(fpgas[f].resUsed, fpgas[f].resUsed + NUM_RES, tmp);
    for (int i = 0; i < NUM_RES; ++i){
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]){
            std::copy(tmp, tmp + NUM_RES, fpgas[f].resUsed);
            return false;
        }
    }
    return true;
}

void ParFunc::_fpga_add_res_force(int f, int u){
    for (int i = 0; i < NUM_RES; ++i){
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
    }
}

// @todo: now full update, maybe in the future we can incrementally update the connection
bool ParFunc::_fpga_cal_conn(int f){
    std::unordered_map<int, bool> netCaled;
    int conn = 0;
    for (int node : fpgas[f].nodes){
        for (int net : nodes[node].nets){
            if (netCaled[net]){
                continue;
            }
            netCaled[net] = true;
            for (int v : nets[net].nodes){
                if (nodes[v].fpga != f){
                    conn += nets[net].weight;
                    if (conn > fpgas[f].maxConn){
                        return false;
                    }
                    break;
                }
            }
        }
    }
    fpgas[f].usedConn = conn;
    return true;
}

void ParFunc::_fpga_cal_conn_force(int f){
    std::unordered_map<int, bool> netCaled;
    int conn = 0;
    for (int node : fpgas[f].nodes){
        for (int net : nodes[node].nets){
            if (netCaled[net]){
                continue;
            }
            netCaled[net] = true;
            for (int v : nets[net].nodes){
                if (nodes[v].fpga != f){
                    conn += nets[net].weight;
                    break;
                }
            }
        }
    }
    fpgas[f].usedConn = conn;
}