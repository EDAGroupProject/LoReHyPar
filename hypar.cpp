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
        for (int i = 0; i < NUM_RES; ++i){
            info >> fpgas.back().resCap[i];
        }
    }
}

void HyPar::readAre(std::ifstream &are){
    std::string name;
    while(are >> name){
        node2id[name] = nodes.size();
        nodes.emplace_back(node{name});
        for (int i = 0; i < NUM_RES; ++i){
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
    for (int i = 0; i < K; ++i){
        fpgaMap[i][i] = 0; // distance to itself is 0, neccessary for floyd-warshall
    }
    std::string name1, name2;
    int id1, id2;
    while (topo >> name1 >> name2){
        id1 = fpga2id[name1];
        id2 = fpga2id[name2];
        fpgaMap[id1][id2] = fpgaMap[id2][id1] = 1;
    }
    for (int k = 0; k < K; k++){
        for (int i = 0; i < K; ++i){
            for (int j = 0; j < K; j++){
                if (fpgaMap[i][j] > fpgaMap[i][k] + fpgaMap[k][j]){
                    fpgaMap[i][j] = fpgaMap[i][k] + fpgaMap[k][j];
                }
            }
        }
    }
    for (int i = 0; i < K; ++i){
            for (int j = 0; j < K; j++){
                if(fpgaMap[i][j] > maxHop){
                    fpgaMap[i][j] = 64; // @warning: a large number to indicate no connection
                }
            }
        }
}

void HyPar::printSummary(std::ostream &out){
    out << "Nodes: " << nodes.size() << std::endl;
    for (size_t i = 0; i < nodes.size(); ++i){
        auto &node = nodes[i];
        out << "  node " << i << ": " << node.name << "-" << std::to_string(node.isRep) << "\n    resLoad: ";
        for (int i = 0; i < NUM_RES; ++i){
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
    for (size_t i = 0; i < nets.size(); ++i){
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
    for (size_t i = 0; i < fpgas.size(); ++i){
        auto &fpga = fpgas[i];
        out << "  fpga " << i << ": " << fpga.name << "\n    maxConn: " << fpga.maxConn << "\n    resCap: ";
        for (int i = 0; i < NUM_RES; ++i){
            out << fpga.resCap[i] << " ";
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
    for (size_t i = 0; i < nodes.size(); ++i){
        auto &node = nodes[i];
        out << "  node " << i << ": " << node.name << "-" << std::to_string(node.isRep) << "\n    resLoad: ";
        for (int i = 0; i < NUM_RES; ++i){
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
    for (size_t i = 0; i < nets.size(); ++i){
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
    for (size_t i = 0; i < fpgas.size(); ++i){
        auto &fpga = fpgas[i];
        out << "  fpga " << i << ": " << fpga.name << "\n    maxConn: " << fpga.maxConn << "\n    resCap: ";
        for (int i = 0; i < NUM_RES; ++i){
            out << fpga.resCap[i] << " ";
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
    contract_memo.push({u, v});
    existing_nodes.erase(v);
    deleted_nodes.insert(v);
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] += nodes[v].resLoad[i];
    }
    for (int net : nodes[v].nets){
        if (nets[net].size == 1){ // v is the only node in the net, relink (u, net)
            nets[net].nodes[0] = u;
            nets[net].source = u;
            nodes[u].nets.insert(net);
            nodes[u].isSou[net] = true;
            // netFp[net] += u * u - v * v; // incrementally update the fingerprint
        }else{
            auto vit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), v);
            if (nodes[v].isSou[net]){
                nets[net].source = u;
                nodes[u].isSou[net] = true;
            }
            if (!nodes[u].nets.count(net)){ // u is not in the net, relink (u, net)
                *vit = u;
                nodes[u].nets.insert(net);
                // netFp[net] += u * u - v * v; // incrementally update the fingerprint
            }
            else{ // u is in the net, delete v
                *vit = nets[net].nodes[--nets[net].size];
                nets[net].nodes[nets[net].size] = v;
                // netFp[net] -= v * v; // incrementally update the fingerprint
            }
        }
    }
}

void ParFunc::_uncontract(int u, int v){
    assert(contract_memo.top() == std::make_pair(u, v));
    contract_memo.pop();
    existing_nodes.insert(v);
    deleted_nodes.erase(v);
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
        if (static_cast<int>(nets[net].nodes.size()) == nets[net].size || nets[net].nodes[nets[net].size] != v){ // v is relinked
            auto uit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u);
            *uit = v;
            nodes[u].nets.erase(net);
        }else{ // v is deleted
            ++nets[net].size;
        }
    }
}

float ParFunc::_heavy_edge_rating(int u, int v){
    if (edge_rating.count({u, v})){
        return edge_rating[{u, v}];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    float rating = 0;
    for (int net : secNets){
        if (nets[net].size > parameter_l){
            continue;
        }
        rating += (float) nets[net].weight / (nets[net].size - 1); // nets[net].size >= 2
    }
    return rating;
}

// void ParFunc::_init_net_fp(){
//     for (size_t i = 0; i < nets.size(); ++i){
//         netFp[i] = 0;
//         for (int node : nets[i].nodes){
//             netFp[i] += node * node;
//         }
//     }
// }

// void ParFunc::_detect_para_singv_net(){
//     // for a deleted net, do we need to record it and plan to recover it?
//     // will deleting a net fail the uncontract operation?
//     // maybe we need to record the order of deletion and the level
//     for (size_t i = 0; i < nets.size(); ++i){
//         if (nets[i].size == 1){
//             netFp[i] = 0; // delete single vertex net
//         }
//     }
//     std::vector<int> indices(nets.size());
//     std::iota(indices.begin(), indices.end(), 0); 
//     std::sort(indices.begin(), indices.end(), [&](int i, int j){return netFp[i] < netFp[j];});
//     bool paraFlag = false;
//     int paraNet = -1, fp = -1;
//     for (int i : indices){
//         if (netFp[i] == 0){
//             continue;
//         }
//         if (!paraFlag){
//             fp = netFp[i];
//             paraNet = i;
//         } else if (netFp[i] == fp){
//             paraFlag = true;
//             nets[paraNet].weight += nets[i].weight; // merge parallel nets
//             netFp[i] = 0;
//         } else{
//             paraFlag = false;
//             fp = -1;
//             paraNet = -1;
//         }
//     }
//     // finally, do we really need this function?
// }

void ParFunc::_init_ceil_mean_res(){
    memset(ceil_rescap, 0, sizeof(ceil_rescap));
    memset(mean_rescap, 0, sizeof(mean_rescap));
    for (int i = 0; i < NUM_RES; ++i){
        for (auto &fpga : fpgas){
            ceil_rescap[i] = std::max(ceil_rescap[i], static_cast<int>(std::ceil((float) fpga.resCap[i] / K * parameter_t)));
            mean_rescap[i] += fpga.resCap[i];
        }
        mean_rescap[i] = static_cast<int>(std::ceil(mean_rescap[i] / K ));
    }
}

bool ParFunc::_contract_eligible(int u, int v){
    for (int i = 0; i < NUM_RES; ++i){
        if (nodes[u].resLoad[i] + nodes[v].resLoad[i] > ceil_rescap[i]){
            return false;
        }
    }
    return true;
}

// @note: the following 3 functions mean that we only check the resource constraint
bool ParFunc::_fpga_add_try(int f, int u){
    int tmp[NUM_RES];
    std::copy(fpgas[f].resUsed, fpgas[f].resUsed + NUM_RES, tmp);
    for (int i = 0; i < NUM_RES; ++i){
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]){
            std::copy(tmp, tmp + NUM_RES, fpgas[f].resUsed);
            return false;
        }
    }
    fpgas[f].resValid = true;
    return true;
}

bool ParFunc::_fpga_add_force(int f, int u){
    fpgas[f].resValid = true;
    for (int i = 0; i < NUM_RES; ++i){
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]){
            fpgas[f].resValid = false;
        }
    }
    return fpgas[f].resValid;
}

bool ParFunc::_fpga_remove_force(int f, int u){
    fpgas[f].resValid = true;
    for (int i = 0; i < NUM_RES; ++i){
        fpgas[f].resUsed[i] -= nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]){
            fpgas[f].resValid = false;
        }
    }
    return fpgas[f].resValid;
}

// @note: is there incremental update for the connectivity?
void ParFunc::_fpga_cal_conn(){
    // fpgaConn.assign(K, 0); // only recalled once, when all 0, ensures the correctness
    // std::unordered_map<std::pair<int, int>, bool, pair_hash> netVis; // unnecessary
    for (const auto &net: nets){
        if (net.size == 1){ // not a cut net
            continue;
        }
        for (auto kvp : net.fpgas){ // a cut net
            fpgas[kvp.first].conn += net.weight;
        }
    }
}

// due to the maxHop, we may not be able to implement the incremental update
void ParFunc::_cal_refine_gain(int node, int f, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map){
    std::unordered_set<int> toFpga;
    for (int net : nodes[node].nets){
        if (nets[net].size == 1){
            continue;
        }
        for (int i = 0; i < nets[net].size; ++i){
            int v = nets[net].nodes[i];
            if (nodes[v].fpga != f){
                toFpga.insert(nodes[v].fpga);
            }
        }
        if (static_cast<int>(toFpga.size()) == K - 1){
            break;
        }
    }
    for (int tf : toFpga){
        int gain = 0;
        bool flag = false;
        for (int net : nodes[node].nets){
            if (nets[net].size == 1){
                continue;
            }
            if (nets[net].source == node){
                for (int j = 0; j < nets[net].size; ++j){
                    int v = nets[net].nodes[j];
                    if (v == node){
                        continue;
                    }
                    int vf = nodes[v].fpga;
                    // @warning: maybe we can simply set the hop of exceeded fpga to a large number
                    gain += nets[net].weight * (fpgaMap[f][vf] - fpgaMap[tf][vf]);
                }
            }else{
                int sf = nodes[nets[net].source].fpga;
                // @warning: maybe we can simply set the hop of exceeded fpga to a large number
                gain += nets[net].weight * (fpgaMap[sf][f] - fpgaMap[sf][tf]);
            }
        }
        if (!flag){
            gain_map[{node, tf}] = gain;
        }
    }
}

void ParFunc::evaluate(){
    for (auto &fpga : fpgas){
        if (fpga.resValid && fpga.conn <= fpga.maxConn){
            std::cout << "Valid FPGA: " << fpga.name << std::endl;
        } else {
            std::cerr << "Invalid FPGA: " << fpga.name << std::endl;
            for (int i = 0; i < NUM_RES; ++i){
                if (fpga.resUsed[i] > fpga.resCap[i]){
                    std::cerr << "Resource " << i << " exceeds the capacity: " << fpga.resUsed[i] << " > " << fpga.resCap[i] << std::endl;
                }
            }
            if (fpga.conn > fpga.maxConn){
                std::cerr << "Connection exceeds the capacity: " << fpga.conn << " > " << fpga.maxConn << std::endl;
            }
        }
    }
    int totalHop = 0;
    for (auto &net : nets){
        int source = net.source;
        int sf = nodes[source].fpga;
        for (int i = 0; i < net.size; ++i){
            int v = net.nodes[i];
            if (v == source){
                continue;
            }
            int vf = nodes[v].fpga;
            if (fpgaMap[sf][vf] == -1){
                std::cerr << "Errors happens between " << nodes[source].name << " and " << nodes[v].name <<"  No path between " << sf << " and " << vf << std::endl;
            } else {
                totalHop += net.weight * fpgaMap[sf][vf]; 
            }   
        }
    }
    std::cout << "Total Hop: " << totalHop << std::endl;
}

void ParFunc::run(){
    preprocess();
    coarsen();
    initial_partition();
    refine();
    std::ofstream out(outputFile);
    printOut(out);
}