#include <hypar.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <climits>
#include <iomanip>
#include <sstream>

HyPar::HyPar(std::string _inputDir, std::string _outputFile) : inputDir(_inputDir), outputFile(_outputFile) {
    std::ifstream info(inputDir + "/design.info"), are(inputDir + "/design.are"), net(inputDir + "/design.net"), topo(inputDir + "/design.topo");
    assert(info && are && net && topo);
    readInfo(info);
    readAre(are);
    readNet(net);
    readTopo(topo);
}

void HyPar::readInfo(std::ifstream &info) {
    std::string name;
    while(info >> name) {
        fpga2id[name] = fpgas.size();
        fpgas.emplace_back(Fpga{name});
        info >> fpgas.back().maxConn;
        for (int i = 0; i < NUM_RES; ++i) {
            info >> fpgas.back().resCap[i];
            ceil_rescap[i] += fpgas.back().resCap[i];
        }
    }
    K = fpgas.size();
    for (int i = 0; i < NUM_RES; ++i) {
        ceil_rescap[i] = ceil_rescap[i] / (K * parameter_t);
    }
}

void HyPar::readAre(std::ifstream &are) {
    std::string name;
    while(are >> name) {
        node2id[name] = nodes.size();
        nodes.emplace_back(Node{name});
        for (int i = 0; i < NUM_RES; ++i) {
            are >> nodes.back().resLoad[i];
        }
    }
    ceil_size = double(nodes.size()) / (K * parameter_t);
}

void HyPar::readNet(std::ifstream &net) {
    std::string line;
    std::string name;
    int id;
    while (std::getline(net, line)) {
        std::istringstream iss(line);
        nets.emplace_back();
        iss >> name;
        id = node2id[name];
        nets.back().nodes.emplace_back(id);
        nets.back().source = id;
        nodes[id].nets.insert(nets.size() - 1);
        nodes[id].isSou[nets.size() - 1] = true;
        nodes[id].fp += (nets.size() - 1) * (nets.size() - 1);
        iss >> nets.back().weight;
        while (iss >> name) {
            id = node2id[name];
            nets.back().nodes.emplace_back(id);
            nodes[id].nets.insert(nets.size() - 1);
            nodes[id].fp += (nets.size() - 1) * (nets.size() - 1);
        }
        nets.back().size = nets.back().nodes.size();
    }
}

void HyPar::readTopo(std::ifstream &topo) {
    topo >> maxHop;
    fpgaMap.assign(K, std::vector<int>(K, maxHop + 1)); // maxHop + 1 means no connection
    for (int i = 0; i < K; ++i) {
        fpgaMap[i][i] = 0; // distance to itself is 0, neccessary for floyd-warshall
    }
    std::string name1, name2;
    int id1, id2;
    while (topo >> name1 >> name2) {
        id1 = fpga2id[name1];
        id2 = fpga2id[name2];
        fpgaMap[id1][id2] = fpgaMap[id2][id1] = 1;
    }
    for (int k = 0; k < K; k++) {
        for (int i = 0; i < K; ++i) {
            for (int j = 0; j < K; j++) {
                if (fpgaMap[i][j] > fpgaMap[i][k] + fpgaMap[k][j]) {
                    fpgaMap[i][j] = fpgaMap[i][k] + fpgaMap[k][j];
                }
            }
        }
    }
    for (int i = 0; i < K; ++i) {
        for (int j = 0; j < K; j++) {
            if(fpgaMap[i][j] > maxHop) {
                fpgaMap[i][j] = K * maxHop; // @warning: a large number to indicate no connection
            } else {
                fpgas[i].neighbors.emplace_back(j);
            }
        }
    }
    for (int i = 0; i < K; ++i) {
        std::sort(fpgas[i].neighbors.begin(), fpgas[i].neighbors.end(), [&](int a, int b) {
            return fpgaMap[i][a] < fpgaMap[i][b];
        });
    }
}

void HyPar::printSummary(std::ostream &out) {
    out << "Nodes: " << nodes.size() << std::endl;
    // for (size_t i = 0; i < nodes.size(); ++i) {
    //     auto &node = nodes[i];
    //     out << "  node " << i << ": " << node.name << "-" << std::to_string(node.isRep) << "\n    resLoad: ";
    //     for (int i = 0; i < NUM_RES; ++i) {
    //         out << node.resLoad[i] << " ";
    //     }
    //     out << "\n    nets: ";
    //     for (auto net : node.nets) {
    //         out << net << " ";
    //     }
    //     out << "\n    reps: ";
    //     for (auto rep : node.reps) {
    //         out << rep << " ";
    //     }
    //     out << "\n    fpga: " << node.fpga << std::endl;
    // }
    out << "Nets: " << nets.size() << std::endl;
    // for (size_t i = 0; i < nets.size(); ++i) {
    //     auto &net = nets[i];
    //     out << "  net " << i << ": \n    weight: ";
    //     out << net.weight << "\n    size: ";
    //     out << net.size << "\n    source: ";
    //     out << net.source << "\n    nodes: ";
    //     for (int node : net.nodes) {
    //         out << node << " ";
    //     }
    //     out << "\n    existing nodes: ";
    //     for (int j = 0; j < net.size; ++j) {
    //         out << net.nodes[j] << " ";
    //     }
    //     out << std::endl;
    // }
    out << "FPGAs: " << fpgas.size() << std::endl;
    for (size_t i = 0; i < fpgas.size(); ++i) {
        auto &fpga = fpgas[i];
        out << "  fpga " << i << ": " << fpga.name << "\n    maxConn: " << fpga.maxConn << "\n    resCap: ";
        for (int i = 0; i < NUM_RES; ++i) {
            out << fpga.resCap[i] << " ";
        }
        out << "\n    nodes: ";
        for (auto node : fpga.nodes) {
            out << node << " ";
        }
        out << std::endl;
    }
    out << "MaxHop: " << maxHop << std::endl;
    out << "FPGA Map:\n";
    for (auto &row : fpgaMap) {
        out << "  ";
        for (auto &cell : row) {
            out << std::setw(3) << std::setfill(' ') << cell << " ";
        }
        out << std::endl;
    }
}

void HyPar::printSummary(std::ofstream &out) {
    out << "Nodes: " << nodes.size() << std::endl;
    for (size_t i = 0; i < nodes.size(); ++i) {
        auto &node = nodes[i];
        out << "  node " << i << ": " << node.name << "-" << std::to_string(node.isRep) << "\n    resLoad: ";
        for (int i = 0; i < NUM_RES; ++i) {
            out << node.resLoad[i] << " ";
        }
        out << "\n    nets: ";
        for (auto net : node.nets) {
            out << net << " ";
        }
        out << "\n    reps: ";
        for (auto rep : node.reps) {
            out << rep << " ";
        }
        out << "\n    fpga: " << node.fpga << std::endl;
    }
    out << "Nets: " << nets.size() << std::endl;
    for (size_t i = 0; i < nets.size(); ++i) {
        auto &net = nets[i];
        out << "  net " << i << ": \n    weight: ";
        out << net.weight << "\n    size: ";
        out << net.size << "\n    source: ";
        out << net.source << "\n    nodes: ";
        for (int node : net.nodes) {
            out << node << " ";
        }
        out << std::endl;
    }
    out << "FPGAs: " << fpgas.size() << std::endl;
    for (size_t i = 0; i < fpgas.size(); ++i) {
        auto &fpga = fpgas[i];
        out << "  fpga " << i << ": " << fpga.name << "\n    maxConn: " << fpga.maxConn << "\n    resCap: ";
        for (int i = 0; i < NUM_RES; ++i) {
            out << fpga.resCap[i] << " ";
        }
        out << "\n    nodes: ";
        for (auto node : fpga.nodes) {
            out << node << " ";
        }
        out << std::endl;
    }
    out << "MaxHop: " << maxHop << std::endl;
    out << "FPGA Map:\n";
    for (auto &row : fpgaMap) {
        out << "  ";
        for (auto &cell : row) {
            out << std::setw(3) << std::setfill(' ') << cell << " ";
        }
        out << std::endl;
    }
}

void HyPar::printOut(std::ofstream &out) {
    for (auto &fpga: fpgas) {
        out << fpga.name << ": ";
        for (auto node : fpga.nodes) {
            if(nodes[node].isRep) {
                out << nodes[node].name << "* ";
            }else{
                out << nodes[node].name << " ";
            }
        }
        out << std::endl;
    }
}

void HyPar::_contract(int u, int v) {
    contract_memo.push({u, v});
    nodes[u].size += nodes[v].size;
    existing_nodes.erase(v);
    deleted_nodes.insert(v);
    for (int i = 0; i < NUM_RES; ++i) {
        nodes[u].resLoad[i] += nodes[v].resLoad[i];
    }
    for (int net : nodes[v].nets) {
        if (nets[net].size == 1) { // v is the only node in the net, relink (u, net)
            nets[net].nodes[0] = u;
            nets[net].source = u;
            nodes[u].nets.insert(net);
            nodes[u].isSou[net] = true;
            nodes[u].fp += net * net;
            // netFp[net] += u * u - v * v; // incrementally update the fingerprint
        }else{
            auto vit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), v);
            if (nodes[v].isSou[net]) {
                nets[net].source = u;
                nodes[u].isSou[net] = true;
            }
            if (!nodes[u].nets.count(net)) { // u is not in the net, relink (u, net)
                *vit = u;
                nodes[u].nets.insert(net);
                nodes[u].fp += net * net;
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

void HyPar::_uncontract(int u, int v) {
    contract_memo.pop();
    nodes[u].size -= nodes[v].size;
    existing_nodes.insert(v);
    deleted_nodes.erase(v);
    for (int i = 0; i < NUM_RES; ++i) {
        nodes[u].resLoad[i] -= nodes[v].resLoad[i];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    for (int net : secNets) {
        if (nodes[v].isSou[net]) {
            nets[net].source = v;
            nodes[u].isSou[net] = false;
        }
        if (static_cast<int>(nets[net].nodes.size()) == nets[net].size || nets[net].nodes[nets[net].size] != v) { // v is relinked
            auto uit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u);
            *uit = v;
            nodes[u].nets.erase(net);
            nodes[u].fp -= net * net;
        }else{ // v is deleted
            ++nets[net].size;
        }
    }
}

void HyPar::_uncontract(int u, int v, int f) {
    contract_memo.pop();
    nodes[u].size -= nodes[v].size;
    existing_nodes.insert(v);
    deleted_nodes.erase(v);
    for (int i = 0; i < NUM_RES; ++i) {
        nodes[u].resLoad[i] -= nodes[v].resLoad[i];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    for (int net : secNets) {
        if (nodes[v].isSou[net]) {
            nets[net].source = v;
            nodes[u].isSou[net] = false;
        }
        if (static_cast<int>(nets[net].nodes.size()) == nets[net].size || nets[net].nodes[nets[net].size] != v) { // v is relinked
            auto uit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u);
            *uit = v;
            nodes[u].nets.erase(net);
            nodes[u].fp -= net * net;
        }else{ // v is deleted
            ++nets[net].size;
            ++nets[net].fpgas[f];
        }
    }
}

float HyPar::_heavy_edge_rating(int u, int v) {
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    float rating = 0;
    for (int net : secNets) {
        if (nets[net].size > parameter_l) {
            continue;
        }
        rating += (float) nets[net].weight / (nets[net].size - 1); // nets[net].size >= 2
    }
    return rating;
}

// u < v
float HyPar::_heavy_edge_rating(int u, int v, std::unordered_map<std::pair<int, int>, int, pair_hash> &rating) {
    if (rating.count({u, v})) {
        return rating[{u, v}];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    if (secNets.empty()) {
        return 0;
    }
    float _rating = 0;
    for (int net : secNets) {
        if (nets[net].size > parameter_l) {
            continue;
        }
        _rating += (float) nets[net].weight / (nets[net].size - 1); // nets[net].size >= 2
    }
    rating[{u, v}] = _rating;
    return _rating;
}

// void HyPar::_init_net_fp() {
//     for (size_t i = 0; i < nets.size(); ++i) {
//         netFp[i] = 0;
//         for (int node : nets[i].nodes) {
//             netFp[i] += node * node;
//         }
//     }
// }

// void HyPar::_detect_para_singv_net() {
//     // for a deleted net, do we need to record it and plan to recover it?
//     // will deleting a net fail the uncontract operation?
//     // maybe we need to record the order of deletion and the level
//     for (size_t i = 0; i < nets.size(); ++i) {
//         if (nets[i].size == 1) {
//             netFp[i] = 0; // delete single vertex net
//         }
//     }
//     std::vector<int> indices(nets.size());
//     std::iota(indices.begin(), indices.end(), 0); 
//     std::sort(indices.begin(), indices.end(), [&](int i, int j) {return netFp[i] < netFp[j];});
//     bool paraFlag = false;
//     int paraNet = -1, fp = -1;
//     for (int i : indices) {
//         if (netFp[i] == 0) {
//             continue;
//         }
//         if (!paraFlag) {
//             fp = netFp[i];
//             paraNet = i;
//         } else if (netFp[i] == fp) {
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

bool HyPar::_contract_eligible(int u, int v) {
    for (int i = 0; i < NUM_RES; ++i) {
        if (nodes[u].resLoad[i] + nodes[v].resLoad[i] > ceil_rescap[i]) {
            return false;
        }
    }
    return (nodes[u].size + nodes[v].size <= ceil_size);
}

// @note: the following 4 functions mean that we only check the resource constraint
bool HyPar::_fpga_add_try(int f, int u) {
    int tmp[NUM_RES];
    std::copy(fpgas[f].resUsed, fpgas[f].resUsed + NUM_RES, tmp);
    for (int i = 0; i < NUM_RES; ++i) {
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]) {
            std::copy(tmp, tmp + NUM_RES, fpgas[f].resUsed);
            return false;
        }
    }
    fpgas[f].resValid = true;
    return true;
}

bool HyPar::_fpga_add_try_nochange(int f, int u) {
    for (int i = 0; i < NUM_RES; ++i) {
        if (fpgas[f].resUsed[i] + nodes[u].resLoad[i] > fpgas[f].resCap[i]) {
            return false;
        }
    }
    return true;
}

bool HyPar::_fpga_add_force(int f, int u) {
    fpgas[f].resValid = true;
    for (int i = 0; i < NUM_RES; ++i) {
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]) {
            fpgas[f].resValid = false;
        }
    }
    return fpgas[f].resValid;
}

bool HyPar::_fpga_remove_force(int f, int u) {
    fpgas[f].resValid = true;
    for (int i = 0; i < NUM_RES; ++i) {
        fpgas[f].resUsed[i] -= nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]) {
            fpgas[f].resValid = false;
        }
    }
    return fpgas[f].resValid;
}

// @note: is there incremental update for the connectivity?
long long HyPar::_fpga_cal_conn() {
    long long conn = 0;
    for (int i = 0; i < K; ++i) {
        fpgas[i].conn = 0;
    }
    for (const auto &net: nets) {
        if (net.size == 1) { // not a cut net
            continue;
        }
        for (auto kvp : net.fpgas) { // a cut net
            if (kvp.second > 0 && kvp.second < net.size) {
                fpgas[kvp.first].conn += net.weight;
                conn += net.weight;
            }
        }
    }
    return conn;
}

bool HyPar::_fpga_chk_conn() {
    for (int i = 0; i < K; ++i) {
        if (fpgas[i].conn > fpgas[i].maxConn) {
            return false;
        }
    }
    return true;
}

bool HyPar::_fpga_chk_res() {
    for (int i = 0; i < K; ++i) {
        if (!fpgas[i].resValid) {
            return false;
        } 
    }
    return true;
}

// GHG gain functions
int HyPar::_max_net_gain(int tf, int u) {
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].fpgas[tf]) {
            gain += nets[net].weight;
        }
    }
    return gain;
}

int HyPar::_FM_gain(int of, int tf, int u) {
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].fpgas[tf] && nets[net].fpgas[tf] == nets[net].size - 1) {
            gain += nets[net].weight;
        }
        if (nets[net].fpgas[of] && nets[net].fpgas[of] == nets[net].size) {
            gain -= nets[net].weight;
        }
    }
    return gain;
}

int HyPar::_connectivity_gain(int of, int tf, int u) {
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (!nets[net].fpgas.count(tf)) {
            gain -= nets[net].weight;
        }
        if (nets[net].fpgas.count(of) && nets[net].fpgas[of] == 1) {
            gain += nets[net].weight;
        }
    }
    return gain;
}

int HyPar::_hop_gain(int of, int tf, int u){
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].source == u) {
            for (int j = 0; j < nets[net].size; ++j) {
                int v = nets[net].nodes[j];
                if (v == u) {
                    continue;
                }
                int vf = nodes[v].fpga;
                // @warning: maybe we can simply set the hop of exceeded fpga to a large number
                gain += nodes[v].size * nets[net].weight * (fpgaMap[of][vf] - fpgaMap[tf][vf]);
            }
        } else {
            int sf = nodes[nets[net].source].fpga;
            // @warning: maybe we can simply set the hop of exceeded fpga to a large number
            gain += nodes[u].size * nets[net].weight * (fpgaMap[sf][of] - fpgaMap[sf][tf]);
        }
    }
    return gain;
}

int HyPar::_gain_function(int of, int tf, int u, int sel){
    switch (sel) {
        case 0:
            return _max_net_gain(tf, u);
        case 1:
            return _FM_gain(of, tf, u);
        case 2:
            return _hop_gain(of, tf, u);
        case 3:
            return _connectivity_gain(of, tf, u);
        default:
            return 0;
    }
}

void HyPar::_cal_inpar_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map){
    for (int tf = 0; tf < K; ++tf) {
        if (tf == f) {
            continue;
        }
        gain_map[{u, tf}] = _gain_function(f, tf, u, sel);
    }
}

// GHG gain functions
void HyPar::_max_net_gain(std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map) {
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        for (int tf : tfs){
            if (nets[net].fpgas[tf]) {
                gain_map[tf] += nets[net].weight;
            }
        }
    }
}

void HyPar::_FM_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map) {
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        for (int tf : tfs) {
            if (nets[net].fpgas[tf] && nets[net].fpgas[tf] == nets[net].size - 1) {
                gain_map[tf] += nets[net].weight;
            }
            if (nets[net].fpgas[tf] && nets[net].fpgas[of] == 1) {
                gain_map[tf] -= nets[net].weight;
            }
        }
    }
}

void HyPar::_connectivity_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map) {
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        for (int tf : tfs) {
            if (nets[net].fpgas[tf] && nets[net].fpgas[tf] == 1) {
                gain_map[tf] += nets[net].weight;
            }
            if (nets[net].fpgas[of] && nets[net].fpgas[of] == 1) {
                gain_map[tf] -= nets[net].weight;
            }
        }
    }
}

void HyPar::_hop_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map){
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].source == u) {
            for (int j = 0; j < nets[net].size; ++j) {
                int v = nets[net].nodes[j];
                if (v == u) {
                    continue;
                }
                int vf = nodes[v].fpga;
                for (int tf : tfs) {
                    gain_map[tf] += nodes[v].size * nets[net].weight * (fpgaMap[of][vf] - fpgaMap[tf][vf]);
                }
            }
        } else {
            int sf = nodes[nets[net].source].fpga;
            for (int tf : tfs) {
                gain_map[tf] += nodes[u].size * nets[net].weight * (fpgaMap[sf][of] - fpgaMap[sf][tf]);
            }
        }
    }
}

void HyPar::_gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map){
    switch (sel) {
        case 0:
            _max_net_gain(tf, u, gain_map);
            break;
        case 1:
            _FM_gain(of, tf, u, gain_map);
            break;
        case 2:
            _hop_gain(of, tf, u, gain_map);
            break;
        case 3:
            _connectivity_gain(of, tf, u, gain_map);
            break;
    }
}

// due to the maxHop, we may not be able to implement the incremental update
void HyPar::_cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map) {
    std::unordered_set<int> toFpga;
    std::unordered_map<int, bool> tofpgamap;
    for (int net : nodes[u].nets) {
        for (auto [ff, cnt] : nets[net].fpgas) {
            if (cnt && ff != f  && ff != -1 && fpgas[ff].resValid && fpgaMap[f][ff] <= maxHop) {
                toFpga.insert(ff);
                tofpgamap[ff] = true;
            } else {
                tofpgamap[ff] = false;
            }
        }
        if (static_cast<int>(tofpgamap.size()) == K) {
            break;
        }
    }
    if (toFpga.empty()) {
        return;
    }
    int maxGain = -INT_MAX, maxF = -1;
    std::unordered_map<int, int> _gain_map;
    _gain_function(f, toFpga, u, sel, _gain_map);
    for (auto [ff, gain] : _gain_map) {
        if (gain > maxGain) {
            maxGain = gain;
            maxF = ff;
        }
    }
    if (maxF != -1) {
        gain_map.push({maxGain, u, maxF});
    }
}

void HyPar::evaluate_summary(std::ostream &out) {
    _fpga_cal_conn();
    for (auto &fpga : fpgas) {
        if (fpga.resValid && fpga.conn <= fpga.maxConn) {
            out << "Valid FPGA: " << fpga.name << std::endl;
        } else {
            out << "Invalid FPGA: " << fpga.name << std::endl;
            for (int i = 0; i < NUM_RES; ++i) {
                if (fpga.resUsed[i] > fpga.resCap[i]) {
                    out << "Resource " << i << " exceeds the capacity: " << fpga.resUsed[i] << " > " << fpga.resCap[i] << std::endl;
                }
            }
            if (fpga.conn > fpga.maxConn) {
                out << "Connection exceeds the capacity: " << fpga.conn << " > " << fpga.maxConn << std::endl;
            }
        }
    }
    long long totalHop = 0;
    for (auto &net : nets) {
        int source = net.source;
        int sf = nodes[source].fpga;
        for (int i = 0; i < net.size; ++i) {
            int v = net.nodes[i];
            if (v == source) {
                continue;
            }
            int vf = nodes[v].fpga;
            if (fpgaMap[sf][vf] > maxHop) {
                out << "Errors happens between " << nodes[source].name << " and " << nodes[v].name <<"  No path between " << sf << " and " << vf << std::endl;
            } else {
                totalHop += nodes[v].size * net.weight * fpgaMap[sf][vf]; 
            }   
        }
    }
    out << "Total Hop: " << totalHop << std::endl;
}

void HyPar::evaluate(bool &valid, long long &hop) {
    valid = true;
    for (auto &fpga : fpgas) {
        if (!fpga.resValid || fpga.conn > fpga.maxConn) {
            valid = false;
        }
    }
    hop = 0;
    for (auto &net : nets) {
        int source = net.source;
        int sf = nodes[source].fpga;
        for (int i = 0; i < net.size; ++i) {
            int v = net.nodes[i];
            if (v == source) {
                continue;
            }
            int vf = nodes[v].fpga;
            if (fpgaMap[sf][vf] > maxHop) {
                valid = false;
            }
            hop += nodes[v].size * net.weight * fpgaMap[sf][vf];
        }
    }
}



void HyPar::run() {
    if (nodes.size() >= 1e6){
        fast_run();
        return;
    } 
    preprocess();
    coarsen();
    initial_partition();
    refine();
    std::ofstream out(outputFile);
    printOut(out);
}

void HyPar::fast_run() {
    auto start = std::chrono::high_resolution_clock::now();

    // Check if a saved preprocess file exists
    std::ifstream inFile("preprocess.txt");
    if (inFile.good()) {
        loadPreprocessResultsAsText("preprocess.txt");
    } else {
        preprocess();
        savePreprocessResultsAsText("preprocess.txt");
    }

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Preprocess time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;

    std::ifstream inFile2("coarsen.txt");
    if (inFile2.good()) {
        loadCoarsenResultsAsText("coarsen.txt");
    } else {
        coarsen();
        saveCoarsenResultsAsText("coarsen.txt");
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Coarsen time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;

    std::ifstream inFile3("initial_partition.txt");
    if (inFile3.good()) {
        loadPartitionResultsAsText("initial_partition.txt");
    } else {
        SCLa_propagation();
        savePartitionResultsAsText("initial_partition.txt");
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Initial partition time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;

    // Output the fpga values of nodes after initial partition
    std::cout << "Nodes' FPGA values after initial partition:" << std::endl;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << "Node " << i << ": FPGA " << nodes[i].fpga << std::endl;
    }

    std::ifstream inFile4("refine.txt");
    if (inFile4.good()) {
        loadRefineResultsAsText("refine.txt");
    } else {
        only_fast_refine();
        saveRefineResultsAsText("refine.txt");
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Refine time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << "s" << std::endl;

    // Proceed to the next steps
    // coarsen();
    // SCLa_propagation();
    // only_fast_refine();
    // std::ofstream out(outputFile);
    // printOut(out);
}
