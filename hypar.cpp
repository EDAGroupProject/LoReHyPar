#include <hypar.hpp>

HyPar::HyPar(std::string _inputDir, std::string _outputFile) : inputDir(_inputDir), outputFile(_outputFile) {
    std::ifstream info(inputDir + "/design.info"), are(inputDir + "/design.are"), net(inputDir + "/design.net"), topo(inputDir + "/design.topo");
    std::unordered_map<std::string, int> node2id;
    std::unordered_map<std::string, int> fpga2id;
    readInfo(info, fpga2id);
    readAre(are, node2id);
    readNet(net, node2id);
    readTopo(topo, fpga2id);
}

void HyPar::readInfo(std::ifstream &info, std::unordered_map<std::string, int> &fpga2id) {
    std::string name;
    fpgas.reserve(64);
    fpga2id.reserve(64);
    while(info >> name) {
        fpga2id[name] = fpgas.size();
        fpgas.emplace_back();
        fpgas.back().name = name;
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

void HyPar::readAre(std::ifstream &are, std::unordered_map<std::string, int> &node2id) {
    std::string name;
    if (K <= 8) {
        nodes.reserve(1e4);
        node2id.reserve(1e4);
    } else if (K <= 32) {
        nodes.reserve(1e5);
        node2id.reserve(1e5);
    } else {
        nodes.reserve(1e6);
        node2id.reserve(1e6);
    }
    while(are >> name) {
        node2id[name] = nodes.size();
        nodes.emplace_back();
        nodes.back().name = name;
        for (int i = 0; i < NUM_RES; ++i) {
            are >> nodes.back().resLoad[i];
            mean_res[i] += nodes.back().resLoad[i];
        }
    }
    for (int i = 0; i < NUM_RES; ++i) {
        mean_res[i] = mean_res[i] / nodes.size();
    }
    ceil_size = double(nodes.size()) / (K * parameter_t);
    existing_nodes.reserve(nodes.size());
    for (size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
}

void HyPar::readNet(std::ifstream &net, std::unordered_map<std::string, int> &node2id) {
    std::string line;
    std::string name;
    int id;
    nets.reserve(10 * nodes.size());
    pin = 0;
    while (std::getline(net, line)) {
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
        pin += nets.back().size;
    }
}

void HyPar::readTopo(std::ifstream &topo, std::unordered_map<std::string, int> &fpga2id) {
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
                fpgaMap[i][j] = K * K * maxHop; // @warning: a large number to indicate no connection
            }
        }
    }
}

void HyPar::printOut() {
    std::ofstream out(outputFile);
    printOut(out);
}

void HyPar::printOut(std::ofstream &out) {
    for (const auto &fpga: fpgas) {
        out << fpga.name << ": ";
        for (int node : fpga.nodes) {
            out << nodes[node].name << " ";
        }
        out << std::endl;
    }
}

void HyPar::printOut(Result &res, long long hop) {
    std::stringstream ss;
    for (const auto &fpga : fpgas) {
        ss << fpga.name << ": ";
        for (int node : fpga.nodes) {
            ss << nodes[node].name << " ";
        }
        ss << std::endl;
    }
    res.output = ss.str();
    res.hop = hop;
}


void HyPar::_contract(int u, int v) {
    contract_memo.push({u, v});
    nodes[u].size += nodes[v].size;
    existing_nodes.erase(v);
    for (int i = 0; i < NUM_RES; ++i) {
        nodes[u].resLoad[i] += nodes[v].resLoad[i];
    }
    for (int net : nodes[v].nets) {
        if (nets[net].size == 1) { // v is the only node in the net, relink (u, net)
            nets[net].nodes[0] = u;
            nets[net].source = u;
            nodes[u].nets.insert(net);
            nodes[u].isSou[net] = true;    
        }else{
            auto vit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), v);
            if (nodes[v].isSou[net]) {
                nets[net].source = u;
                nodes[u].isSou[net] = true;
            }
            if (!nodes[u].nets.count(net)) { // u is not in the net, relink (u, net)
                *vit = u;
                nodes[u].nets.insert(net);
            }
            else{ // u is in the net, delete v
                *vit = nets[net].nodes[--nets[net].size];
                nets[net].nodes[nets[net].size] = v;
            }
        }
    }
}

void HyPar::_uncontract(int u, int v, int f) {
    contract_memo.pop();
    nodes[u].size -= nodes[v].size;
    nodes[v].fpga = f;
    fpgas[f].nodes.insert(v);
    existing_nodes.insert(v);
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
        }else{ // v is deleted
            ++nets[net].size;
            ++nets[net].fpgas[f];
        }
    }
}

bool HyPar::_contract_eligible(int u, int v) {
    for (int i = 0; i < NUM_RES; ++i) {
        if (nodes[u].resLoad[i] + nodes[v].resLoad[i] > ceil_rescap[i]) {
            return false;
        }
    }
    return (nodes[u].size + nodes[v].size <= ceil_size);
}

bool HyPar::_fpga_add_try(int f, int u) {
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

int HyPar::_hop_gain(int of, int tf, int u){
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].source == u) {
            for (const auto &[f, cnt] : nets[net].fpgas) {
                if (cnt == 0) {
                    continue;
                }
                if (f == of) {
                    if (cnt > 1) {
                        gain -= nets[net].weight * fpgaMap[of][tf];
                    }
                } else if (f == tf) {
                    gain += nets[net].weight * fpgaMap[of][tf];
                } else {
                    gain += nets[net].weight * (fpgaMap[of][f] - fpgaMap[tf][f]);
                }
            }
        } else {
            int sf = nodes[nets[net].source].fpga;
            if (nets[net].fpgas[of] == 1) {
                gain += nets[net].weight * (fpgaMap[sf][of] - fpgaMap[sf][tf]);
            }
            if (nets[net].fpgas[tf] == 0) {
                gain -= nets[net].weight * (fpgaMap[sf][of] - fpgaMap[sf][tf]);
            }
        }
    }
    return gain;
}

int HyPar::_gain_function(int of, int tf, int u, int sel){
    switch (sel) {
        case 0:
            return _hop_gain(of, tf, u);
        case 1:
            return _FM_gain(of, tf, u);
        default:
            return 0;
    }
}

void HyPar::_cal_gain(int u, int f, int sel, std::unordered_map<std::pair<int, int>, int, pair_hash> &gain_map){
    std::unordered_map<int, bool> toFpga;
    _get_eligible_fpga(u, toFpga);
    for (int tf = 0; tf < K; ++tf) {
        if (toFpga[tf]) {
            gain_map[{u, tf}] = _gain_function(f, tf, u, sel);
        } else if (gain_map.count({u, tf})) {
            gain_map.erase({u, tf});
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

void HyPar::_hop_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map){
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].source == u) {
            for (const auto &[f, cnt] : nets[net].fpgas) {
                if (cnt == 0) {
                    continue;
                }
                for (int tf : tfs) {
                    if (f == of) {
                        if (cnt > 1) {
                            gain_map[tf] -= nets[net].weight * fpgaMap[of][tf];
                        }
                    } else if (f == tf) {
                        gain_map[tf] += nets[net].weight * fpgaMap[of][tf];
                    } else {
                        gain_map[tf] += nets[net].weight * (fpgaMap[of][f] - fpgaMap[tf][f]);
                    }
                }
            }
        } else {
            int sf = nodes[nets[net].source].fpga;
            for (int tf : tfs) {
                if (nets[net].fpgas[of] == 1) {
                    gain_map[tf] += nets[net].weight * (fpgaMap[sf][of] - fpgaMap[sf][tf]);
                }
                if (nets[net].fpgas[tf] == 0) {
                    gain_map[tf] -= nets[net].weight * (fpgaMap[sf][of] - fpgaMap[sf][tf]);
                }
            }
        }
    }
}

void HyPar::_gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map){
    switch (sel) {
        case 0:
            _hop_gain(of, tf, u, gain_map);
            break;
        case 1:
            _FM_gain(of, tf, u, gain_map);
            break;
    }
}

// due to the maxHop, we may not be able to implement the incremental update
void HyPar::_cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map) {
    std::unordered_set<int> toFpga;
    _get_eligible_fpga(u, toFpga);
    if (toFpga.empty()) {
        return;
    }
    std::unordered_map<int, int> _gain_map;
    _gain_function(f, toFpga, u, sel, _gain_map);
    for (const auto &[ff, gain] : _gain_map) {
        gain_map.push({gain, u, ff});
    }
}

void HyPar::_get_eligible_fpga(int u, std::unordered_set<int> &tfs) {
    int uf = nodes[u].fpga;
    for (int f = 0; f < K; ++f) {
        if (f != uf && _fpga_add_try(f, u)) {
            tfs.insert(f);
        }
    }
    for (int net : nodes[u].nets) {
        int s = nets[net].source;
        if (s == u) {
            for (const auto &[f, cnt] : nets[net].fpgas) {
                if (cnt == 0 || (f == uf && nets[net].fpgas[f] == 1)) {
                    continue;
                }
                for (auto it = tfs.begin(); it != tfs.end();) {
                    int tf = *it;
                    if (fpgaMap[tf][f] > maxHop) {
                        it = tfs.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        } else {
            int sf = nodes[s].fpga;
            if (sf == -1) {
                continue;
            }
            for (auto it = tfs.begin(); it != tfs.end();) {
                int tf = *it;
                if (fpgaMap[sf][tf] > maxHop) {
                    it = tfs.erase(it);
                } else {
                    ++it;
                }
            }
        }
    }
}

void HyPar::_get_eligible_fpga(int u, std::unordered_map<int, bool> &tfs) {
    int uf = nodes[u].fpga;
    for (int f = 0; f < K; ++f) {
        if (f != uf && _fpga_add_try(f, u)) {
            tfs[f] = true;
        } else {
            tfs[f] = false;
        }
    }
    for (int net : nodes[u].nets) {
        int s = nets[net].source;
        if (s == u) {
            for (const auto &[f, cnt] : nets[net].fpgas) {
                if (cnt == 0 || (f == uf && nets[net].fpgas[f] == 1)) {
                    continue;
                }
                for (int tf = 0; tf < K; ++tf) {
                    if (fpgaMap[tf][f] > maxHop) {
                        tfs[tf] = false;
                    }
                }
            }
        } else {
            int sf = nodes[s].fpga;
            if (sf == -1) {
                continue;
            }
            for (int tf = 0; tf < K; ++tf) {
                if (fpgaMap[sf][tf] > maxHop) {
                    tfs[tf] = false;
                }
            }
        }
    }
}

void HyPar::evaluate(bool &valid, long long &hop) {
    valid = true;
    for (int f = 0; f < K; ++f) {
        fpgas[f].conn = 0;
    }
    hop = 0;
    for (const auto &net : nets) {
        int source = net.source;
        int sf = nodes[source].fpga;
        for (const auto &[f, cnt] : net.fpgas) {
            if (!cnt) {
                continue;
            }
            if (cnt < net.size) {
                fpgas[f].conn += net.weight;
            }
            if (fpgaMap[sf][f] > maxHop) {
                valid = false;
            }
            hop += net.weight * fpgaMap[sf][f];
        }
    }
    valid = true;
    for (const auto &fpga : fpgas) {
        if (!fpga.resValid || fpga.conn > fpga.maxConn) {
            valid = false;
        }
    }
}

void HyPar::run(bool &valid, long long &hop) {
    if (pin <= 1e2) {
        initial_partition();
        refine();
    } else if (pin <= 2e5) {
        coarsen();
        fast_initial_partition();
        fast_refine();
    } else {
        coarsen();
        SCLa_propagation();
        activate_max_hop_nodes(0);
        only_fast_refine();
    }
    evaluate(valid, hop);
}

void HyPar::run_nc(bool &valid, long long &hop) {
    if (pin <= 1e2) {
        initial_partition();
        refine();
    } else if (pin <= 2e5) {
        fast_initial_partition();
        fast_refine();
    } else {
        SCLa_propagation();
        activate_max_hop_nodes(0);
        only_fast_refine();
    }
    evaluate(valid, hop);
}
