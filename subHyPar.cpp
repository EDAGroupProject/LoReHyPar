#include <subHyPar.hpp>
#include <algorithm>
#include <cassert>
#include <cstring>
#include <climits>
#include <iomanip>
#include <sstream>

subHyPar::subHyPar(const HyPar &hp, const std::unordered_set<int> &hp_nodes) {
    maxHop = hp.maxHop;
    K = hp.K;
    fpgas = hp.fpgas;
    fpgaMap = hp.fpgaMap;
    parameter_t = hp.parameter_t;
    parameter_l = hp.parameter_l;
    parameter_tau = hp.parameter_tau;
    existing_nodes = hp_nodes;
    for (int i : hp_nodes) {
        nodes[i] = hp.nodes[i];
        nodes[i].nets.clear();
        nodes[i].size = 1;
        for (int j = 0; j < NUM_RES; ++j) {
            ceil_rescap[j] += nodes[i].resLoad[j];
        }
    }
    for (int i = 0; i < NUM_RES; ++i) {
        ceil_rescap[i] = static_cast<int>(std::ceil(ceil_rescap[i] / (K * parameter_t)));
    }
    ceil_size = static_cast<int>(std::ceil(double(hp_nodes.size()) / (K * parameter_t)));
    for (size_t i = 0; i < hp.nets.size(); ++i) {
        const auto &net = hp.nets[i];
        if (!hp_nodes.count(net.source)) {
            continue;
        }
        Net tmp;
        for (int node : net.nodes) {
            if (hp_nodes.count(node)) {
                tmp.nodes.push_back(node);
                nodes[node].nets.insert(i);
                ++tmp.size;
            }
        }
        if (tmp.size > 1) {
            tmp.weight = net.weight;
            tmp.source = net.source;
            nets[i] = std::move(tmp);
        } else {
            nodes[tmp.nodes[0]].nets.erase(i);
        }
        // @todo: maybe we can use replicated source here
        // for (int node : net.nodes) {
        //     if (hp_nodes.count(node)) {
        //         tmp.nodes.push_back(node);
        //     }
        // }
        // if (tmp.nodes.size() > 1) {
        //     nets[i] = tmp;
        //     tmp.weight = net.weight;
        //     tmp.source = net.source;
        // }
    }
}

void subHyPar::_contract(int u, int v) {
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

void subHyPar::_uncontract(int u, int v) {
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

void subHyPar::_uncontract(int u, int v, int f) {
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

float subHyPar::_heavy_edge_rating(int u, int v) {
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
float subHyPar::_heavy_edge_rating(int u, int v, std::unordered_map<std::pair<int, int>, int, pair_hash> &rating) {
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

bool subHyPar::_contract_eligible(int u, int v) {
    for (int i = 0; i < NUM_RES; ++i) {
        if (nodes[u].resLoad[i] + nodes[v].resLoad[i] > ceil_rescap[i]) {
            return false;
        }
    }
    return (nodes[u].size + nodes[v].size <= ceil_size);
}

// @note: the following 4 functions mean that we only check the resource constraint
bool subHyPar::_fpga_add_try(int f, int u) {
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

bool subHyPar::_fpga_add_try_nochange(int f, int u) {
    for (int i = 0; i < NUM_RES; ++i) {
        if (fpgas[f].resUsed[i] + nodes[u].resLoad[i] > fpgas[f].resCap[i]) {
            return false;
        }
    }
    return true;
}

bool subHyPar::_fpga_add_force(int f, int u) {
    fpgas[f].resValid = true;
    for (int i = 0; i < NUM_RES; ++i) {
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]) {
            fpgas[f].resValid = false;
        }
    }
    return fpgas[f].resValid;
}

bool subHyPar::_fpga_remove_force(int f, int u) {
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
void subHyPar::_fpga_cal_conn() {
    for (int i = 0; i < K; ++i) {
        fpgas[i].conn = 0;
    }
    for (const auto &[_, net]: nets) {
        if (net.size == 1) { // not a cut net
            continue;
        }
        for (auto kvp : net.fpgas) { // a cut net
            if (kvp.second) {
                fpgas[kvp.first].conn += net.weight;
            }
        }
    }
}

bool subHyPar::_fpga_chk_conn() {
    for (int i = 0; i < K; ++i) {
        if (fpgas[i].conn > fpgas[i].maxConn) {
            return false;
        }
    }
    return true;
}

bool subHyPar::_fpga_chk_res() {
    for (int i = 0; i < K; ++i) {
        if (!fpgas[i].resValid) {
            return false;
        } 
    }
    return true;
}

// GHG gain functions
int subHyPar::_max_net_gain(int tf, int u) {
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].fpgas.count(tf)) {
            gain += nets[net].weight;
        }
    }
    return gain;
}

int subHyPar::_FM_gain(int of, int tf, int u) {
    int gain = 0;
    for (int net : nodes[u].nets) {
        if (nets[net].size == 1) {
            continue;
        }
        if (nets[net].fpgas.count(tf) && nets[net].fpgas[tf] == nets[net].size - 1) {
            gain += nets[net].weight;
        }
        if (nets[net].fpgas.count(of) && nets[net].fpgas[of] == nets[net].size) {
            gain -= nets[net].weight;
        }
    }
    return gain;
}

int subHyPar::_connectivity_gain(int of, int tf, int u) {
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

int subHyPar::_hop_gain(int of, int tf, int u){
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

int subHyPar::_gain_function(int of, int tf, int u, int sel){
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

// GHG gain functions
void subHyPar::_max_net_gain(std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map) {
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

void subHyPar::_FM_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map) {
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

void subHyPar::_connectivity_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map) {
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

void subHyPar::_hop_gain(int of, std::unordered_set<int> &tfs, int u, std::unordered_map<int, int> &gain_map){
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

void subHyPar::_gain_function(int of, std::unordered_set<int> &tf, int u, int sel, std::unordered_map<int, int> &gain_map){
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
void subHyPar::_cal_gain(int u, int f, int sel, std::priority_queue<std::tuple<int, int, int>> &gain_map) {
    std::unordered_set<int> toFpga;
    for (int net : nodes[u].nets) {
        for (auto [ff, cnt] : nets[net].fpgas) {
            if (cnt && ff != f  && fpgas[ff].resValid && fpgaMap[f][ff] <= maxHop) {
                toFpga.insert(ff);
            }
        }
        if (static_cast<int>(toFpga.size()) == K - 1) {
            break;
        }
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
    gain_map.push({maxGain, u, maxF});
}

void subHyPar::evaluate(bool &valid, long long &hop) {
    valid = true;
    for (auto &fpga : fpgas) {
        if (!fpga.resValid || fpga.conn > fpga.maxConn) {
            valid = false;
        }
    }
    hop = 0;
    for (auto &[_, net] : nets) {
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

void subHyPar::run() {
    coarsen_naive();
    initial_partition();
    refine();
}

bool subHyPar::_coarsen_naive() {
    bool contFlag = false;
    std::vector<int> randNodes(existing_nodes.begin(), existing_nodes.end());
    std::shuffle(randNodes.begin(), randNodes.end(), get_rng());
    for (int u : randNodes) {
        if (deleted_nodes.count(u)) {
            continue;
        }
        float maxRating = 0.0;
        int v = -1;
        for (int net : nodes[u].nets) {
            for (int i = 0; i < nets[net].size; ++i) {
                int w = nets[net].nodes[i];
                if (w == u || deleted_nodes.count(w) || !_contract_eligible(u, w)) {
                    continue;
                }
                float rating = _heavy_edge_rating(u, w);
                if (rating > maxRating) {
                    maxRating = rating;
                    v = w;
                }
            }
        }
        if (v != -1) {
            _contract(u, v);
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                                                                            
}

void subHyPar::coarsen_naive() {
    while (existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
        if (!_coarsen_naive()) {
            break;
        }
    }
}

void subHyPar::bfs_partition() {
    std::vector<int> nodeVec(existing_nodes.begin(), existing_nodes.end());
    std::mt19937 rng = get_rng();
    std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
    std::vector<int> fpgaVec(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), rng);
    std::uniform_int_distribution<int> dis(0, K - 1);
    for (auto it = nodeVec.begin(); it != nodeVec.end();) {
        if (nodes[*it].fpga != -1) {
            it = nodeVec.erase(it);
            continue;
        } 
        int cur = dis(rng);
        int f = fpgaVec[cur];
        if (_fpga_add_try(f, *it)) {
            std::queue<int> q;
            q.push(*it);
            while (!q.empty()) {
                int u = q.front();
                q.pop();
                nodes[u].fpga = f;
                fpgas[f].nodes.push_back(u);
                for (int net : nodes[u].nets) {
                    ++nets[net].fpgas[f]; // ready for the connectivity calculation
                    for (int i = 0; i < nets[net].size; ++i) {
                        int v = nets[net].nodes[i];
                        if (nodes[v].fpga == -1 && _fpga_add_try(f, v)) {
                            q.push(v);
                        }
                    }
                }
            }
            it = nodeVec.erase(it);
        } else {
            ++it;
        }
    }
    if (!nodeVec.empty()) {
        for(auto node : nodeVec) {
            int cur = dis(rng); // @warning: it's too arbitrary
            int f = fpgaVec[cur];
            nodes[node].fpga = f;
            fpgas[f].nodes.push_back(node);
            _fpga_add_force(f, node);
            for (int net : nodes[node].nets) {
                ++nets[net].fpgas[f];
            }
        }
    }
}

void subHyPar::SCLa_propagation() {
    std::unordered_set<int> unlabeled_nodes(existing_nodes);
    std::vector<int> nodeVec(existing_nodes.begin(), existing_nodes.end());
    std::mt19937 rng = get_rng();
    std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
    int step = (nodeVec.size() - 1) / K;
    for (int i = 0; i < K; ++i) {
        int u = nodeVec[i * step];
        unlabeled_nodes.erase(u);
        nodes[u].fpga = i;
        fpgas[i].nodes.push_back(u);
        _fpga_add_force(i, u);
        int cnt = 0;
        for (int net : nodes[u].nets) {
            ++nets[net].fpgas[i];
            if (cnt >= parameter_tau) {
                continue;
            }
            for (int j = 0; j < nets[net].size; ++j) {
                int v = nets[net].nodes[j];
                if (cnt >= parameter_tau) {
                    break;
                }
                if (nodes[v].fpga == -1 && _fpga_add_try(i, v)) {
                    ++cnt;
                    unlabeled_nodes.erase(v);
                    nodes[v].fpga = i;
                    fpgas[i].nodes.push_back(v);
                    for (int vnet : nodes[v].nets) {
                        ++nets[vnet].fpgas[i];
                    }
                }
            }
        }
    }
    for (int i = 0; i < K ; ++i) {
        for (auto it = unlabeled_nodes.begin(); it != unlabeled_nodes.end();) {
            int u = *it;
            std::unordered_map<int, int> gain_map;
            for (int net : nodes[u].nets) {
                for (auto [f, cnt] : nets[net].fpgas) {
                    if (cnt == 0) {
                        continue;
                    }
                    gain_map[f] += cnt * nets[net].weight; // @warning: the gain is arbitrary
                }
            }
            if (gain_map.empty()) {
                ++it;
                continue;
            }
            int f_max = -1, gain_max = 0;
            for (auto [f, g] : gain_map) {
                if (g > gain_max && _fpga_add_try_nochange(f, u)) {
                    f_max = f;
                    gain_max = g;
                }
            }
            if (f_max == -1) {
                std::uniform_int_distribution<int> dis(0, gain_map.size() - 1);
                bool steps = dis(rng);
                auto it = gain_map.begin();
                advance(it, steps);
                f_max = it->first;
            }
            nodes[u].fpga = f_max;
            fpgas[f_max].nodes.push_back(u);
            _fpga_add_force(f_max, u);
            it = unlabeled_nodes.erase(it);
            for (int net : nodes[u].nets) {
                ++nets[net].fpgas[f_max];
            }
        }
    }
    std::uniform_int_distribution<int> dis(0, K - 1);
    if (!unlabeled_nodes.empty()) {
        for(auto node : unlabeled_nodes) {
            int f = dis(rng);
            nodes[node].fpga = f;
            fpgas[f].nodes.push_back(node);
            _fpga_add_force(f, node);
            for (int net : nodes[node].nets) {
                ++nets[net].fpgas[f];
            }
        }
    }
}

// maybe consider global and sequential later
// for now, I copied the refine.cpp to here, which definitely needs to be modified
void subHyPar::greedy_hypergraph_growth(int sel) { // I use this based on the former initial partition results
    std::vector<std::pair<int,int>> move_seq; // (node, from_fpga)
    std::unordered_map<int, int> node_state; // 0: active, 1: locked
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes(existing_nodes);
    for (int node : existing_nodes) {
        _cal_gain(node, nodes[node].fpga, sel, gain_map);
    }
    int gain_sum = 0, cnt = 0, min_pass = std::log(existing_nodes.size()) / std::log(2);
    int max_sum = -INT_MAX, max_pass = 0;
    while ((cnt < min_pass || gain_sum > 0) && !gain_map.empty()) {
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        while (node_state[max_n] == 1 && !gain_map.empty()) {
            std::tie(max_gain, max_n, max_tf) = gain_map.top();
            gain_map.pop();
        }
        if (gain_map.empty()) {
            break;
        }
        if (max_gain <= 0) {
            ++cnt;
        } else {
            cnt = 0;
        }
        int of = nodes[max_n].fpga;
        move_seq.emplace_back(max_n, of);
        gain_sum += max_gain;
        if (gain_sum > max_sum) {
            max_sum = gain_sum;
            max_pass = static_cast<int>(move_seq.size() - 1);
        }
        fpgas[of].nodes.erase(std::remove(fpgas[of].nodes.begin(), fpgas[of].nodes.end(), max_n), fpgas[of].nodes.end());
        _fpga_remove_force(of, max_n);
        nodes[max_n].fpga = max_tf;
        fpgas[max_tf].nodes.push_back(max_n);
        _fpga_add_force(max_tf, max_n);
        node_state[max_n] = 2;
        active_nodes.erase(max_n);
        for (int net : nodes[max_n].nets) {
            --nets[net].fpgas[of];
            ++nets[net].fpgas[max_tf];
            for (int j = 0; j < nets[net].size; ++j) {
                int v = nets[net].nodes[j];
                if (node_state[v]) {
                    continue;
                }
                _cal_gain(v, nodes[v].fpga, sel, gain_map);
                node_state[v] = 1;
            }
        }
    }
    // now we go back to the best solution
    for (int i = static_cast<int>(move_seq.size()) - 1; i > max_pass; --i) {
        auto [node, of] = move_seq[i];
        int tf = nodes[node].fpga;
        fpgas[tf].nodes.erase(std::remove(fpgas[tf].nodes.begin(), fpgas[tf].nodes.end(), node), fpgas[tf].nodes.end());
        _fpga_remove_force(tf, node);
        nodes[node].fpga = of;
        fpgas[of].nodes.push_back(node);
        _fpga_add_force(of, node);
        for (int net : nodes[node].nets) {
            --nets[net].fpgas[tf];
            ++nets[net].fpgas[of];
        }
    }
}

// @todo: other partitioning methods to enrich the portfolio
void subHyPar::initial_partition() {
    bool bestvalid = false, valid;
    long long minHop = LONG_LONG_MAX, hop;
    subHyPar best, tmp;
    for (int i = 0; i < 20; ++i){
        tmp = *this;
        tmp.bfs_partition();
        tmp._fpga_cal_conn();
        tmp.evaluate(valid, hop);
        if ((!bestvalid || valid) && hop < minHop) {
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
        std::cout << "BFS Partition: " << hop << std::endl;
    }
    for (int i = 0; i < 20; ++i){
        tmp = *this;
        tmp.SCLa_propagation();
        tmp._fpga_cal_conn();
        tmp.evaluate(valid, hop);
        if ((!bestvalid || valid) && hop < minHop) {
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
        std::cout << "SCLa Partition: " << hop << std::endl;
    }
    bool flag = true;
    while (flag) {
        flag = false;
        tmp = *this;
        tmp.bfs_partition();
        tmp._fpga_cal_conn();
        tmp.evaluate(valid, hop);
        if ((!bestvalid || valid) && hop < minHop) {
            flag = true;
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
        std::cout << "BFS Partition: " << hop << std::endl;
        tmp = *this;
        tmp.SCLa_propagation();
        tmp._fpga_cal_conn();
        tmp.evaluate(valid, hop);
        if ((!bestvalid || valid) && hop < minHop){
            flag = true;
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
        std::cout << "SCLa Partition: " << hop << std::endl;
    }
    tmp = best;
    flag = true;
    while (flag) {
        flag = false;
        for (int i = 1; i < 4; ++i){
            tmp.greedy_hypergraph_growth(i);
            tmp._fpga_cal_conn();
            tmp.evaluate(valid, hop);
            if ((!bestvalid || valid) && hop < minHop) {
                flag = true;
                best = tmp;
                bestvalid = valid;
                minHop = hop;
            }
            std::cout << "Greedy Hypergraph Growth: " << hop << std::endl;
        }
    }
    *this = std::move(best);
    std::cout << "Initial Partition: " << minHop << std::endl;
}

void subHyPar::k_way_localized_refine(int sel) {
    auto [u, v] = contract_memo.top();
    int f = nodes[u].fpga;
    nodes[v].fpga = f;
    fpgas[f].nodes.push_back(v);
    _uncontract(u, v, f);
    std::vector<std::pair<int,int>> move_seq; // (node, from_fpga)
    std::unordered_map<int, int> node_state; // 0: inactive, 1: active, 2: locked
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes;
    node_state[u] = 1;
    node_state[v] = 1;
    active_nodes.insert(u);
    active_nodes.insert(v);
    _cal_gain(u, nodes[u].fpga, sel, gain_map);
    _cal_gain(v, nodes[v].fpga, sel, gain_map);
    int gain_sum = 0, cnt = 0, min_pass = std::log(existing_nodes.size()) / std::log(2);
    int max_sum = -INT_MAX, max_pass = 0;
    while ((cnt < min_pass || gain_sum > 0) && !gain_map.empty()) {
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        while (node_state[max_n] == 2 && !gain_map.empty()) {
            std::tie(max_gain, max_n, max_tf) = gain_map.top();
            gain_map.pop();
        }
        if (gain_map.empty()) {
            break;
        }
        if (max_gain <= 0) {
            ++cnt;
        } else {
            cnt = 0;
        }
        int of = nodes[max_n].fpga;
        move_seq.emplace_back(max_n, of);
        gain_sum += max_gain;
        if (gain_sum > max_sum) {
            max_sum = gain_sum;
            max_pass = static_cast<int>(move_seq.size() - 1);
        }
        fpgas[of].nodes.erase(std::remove(fpgas[of].nodes.begin(), fpgas[of].nodes.end(), max_n), fpgas[of].nodes.end());
        _fpga_remove_force(of, max_n);
        nodes[max_n].fpga = max_tf;
        fpgas[max_tf].nodes.push_back(max_n);
        _fpga_add_force(max_tf, max_n);
        node_state[max_n] = 2;
        active_nodes.erase(max_n);
        for (int net : nodes[max_n].nets) {
            --nets[net].fpgas[of];
            ++nets[net].fpgas[max_tf];
            for (int j = 0; j < nets[net].size; ++j) {
                int v = nets[net].nodes[j];
                if (node_state[v] >= 1) {
                    continue;
                }
                _cal_gain(v, nodes[v].fpga, sel, gain_map);
                node_state[v] = 1;
            }
        }
    }
    // now we go back to the best solution
    for (int i = static_cast<int>(move_seq.size()) - 1; i > max_pass; --i) {
        auto [node, of] = move_seq[i];
        int tf = nodes[node].fpga;
        fpgas[tf].nodes.erase(std::remove(fpgas[tf].nodes.begin(), fpgas[tf].nodes.end(), node), fpgas[tf].nodes.end());
        _fpga_remove_force(tf, node);
        nodes[node].fpga = of;
        fpgas[of].nodes.push_back(node);
        _fpga_add_force(of, node);
        for (int net : nodes[node].nets) {
            --nets[net].fpgas[tf];
            ++nets[net].fpgas[of];
        }
    }
}

bool subHyPar::force_validity_refine(int sel) {
    std::unordered_set<int> invalid_fpgas;
    std::unordered_map<int, bool> node_locked;
    std::vector<double> fpgaRes(K, 0);
    for (int i = 0; i < K; ++i) {
        if (!fpgas[i].resValid) {
            invalid_fpgas.insert(i);
            for (int j = 0; j < NUM_RES; ++j) {
                fpgaRes[i] += double(fpgas[i].resUsed[j]) / fpgas[i].resCap[j]; 
            }
        }
    }
    if (invalid_fpgas.empty()) {
        return true;
    }
    std::vector<int> invalid_fpgaVec(invalid_fpgas.begin(), invalid_fpgas.end());
    std::sort(invalid_fpgaVec.begin(), invalid_fpgaVec.end(), [&](int a, int b) {
        return fpgaRes[a] > fpgaRes[b];
    });
    for (int f : invalid_fpgaVec) {
        int ceil = fpgas[f].nodes.size() / K;
        std::mt19937 rng = get_rng();
        for (int i = 0; i < ceil; ++i) {
            std::uniform_int_distribution<int> dis(0, static_cast<int>(fpgas[f].nodes.size()) - 1);
            int u = fpgas[f].nodes[dis(rng)];
            if (node_locked[u]) {
                --i;
                continue;
            }
            node_locked[u] = true;
            int max_gain = -INT_MAX, max_tf;
            for (int tf = 0; tf < K; ++tf) {
                if (tf == f || !fpgas[tf].resValid || !_fpga_add_try_nochange(tf, u)) {
                    continue;
                }
                int gain = _gain_function(f, tf, u, sel);
                if (gain > max_gain) {
                    max_gain = gain;
                    max_tf = tf;
                }
            }
            fpgas[f].nodes.erase(std::remove(fpgas[f].nodes.begin(), fpgas[f].nodes.end(), u), fpgas[f].nodes.end());
            _fpga_remove_force(f, u);
            nodes[u].fpga = max_tf;
            fpgas[max_tf].nodes.push_back(u);
            _fpga_add_force(max_tf, u);
            for (int net : nodes[u].nets) {
                ++nets[net].fpgas[max_tf];
                --nets[net].fpgas[f];
            }
        }
    }
    return false;
}

void subHyPar::refine() {
    while (!contract_memo.empty()) {
        auto start = std::chrono::high_resolution_clock::now();
        k_way_localized_refine(2);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "refine time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    }
    if (!_fpga_chk_res()) {
        for (int i = 0; i < K; ++i) {
            if(force_validity_refine(2)) {
                break;
            }
        }
        for (int i = 0; i < K; ++i) {
            if(force_validity_refine(3)) {
                break;
            }
        }
    }
    // @note: how to ensure the validity of the solution?
    // Since the organizer declares that getting a valid solution is easy, we just randomly assign nodes to fpgas
}