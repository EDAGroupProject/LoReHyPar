#include <hypar.hpp>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cassert>

void HyPar::refine_naive() {
    // @warning: the following code is just uncontract all nodes
    while (!contract_memo.empty()) {
        auto [u, v] = contract_memo.top();
        _uncontract(u, v);
        int f = nodes[u].fpga;
        nodes[v].fpga = f;
        fpgas[f].nodes.insert(v);
        for (int net : nodes[v].nets) {
            ++nets[net].fpgas[f];
        }
    }
}

// @warning: Currently, we do not consider the case when the move enlarges the choice set
// so we need to calculate the gain of the move each time we call this function
// if we can take this into account, we will not need to calculate the gain each time
// @todo: use different gain function to improve both hop and connectivity
void HyPar::k_way_localized_refine(int sel) {
    auto [u, v] = contract_memo.top();
    int f = nodes[u].fpga;
    _uncontract(u, v, f);
    std::vector<std::pair<int,int>> move_seq; // (node, from_fpga)
    std::unordered_map<int, int> node_state; // 0: inactive, 1: active, 2: locked
    std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map; // (node, fpga) -> gain
    std::unordered_set<int> active_nodes;
    node_state[u] = 1;
    node_state[v] = 1;
    active_nodes.insert(u);
    active_nodes.insert(v);
    _cal_gain(u, f, sel, gain_map);
    _cal_gain(v, f, sel, gain_map);
    int gain_sum = 0, cnt = 0, min_pass = std::log(existing_nodes.size()) / std::log(2);
    int max_sum = -INT_MAX, max_pass = -1;
    while ((cnt < min_pass || gain_sum > 0) && !active_nodes.empty()) {
        int max_gain = -INT_MAX, max_n = -1, max_tf = -1;
        for (int u : active_nodes) {
            for (int f = 0; f < K; ++f) {
                if (gain_map.count({u, f}) && gain_map[{u, f}] > max_gain && _fpga_add_try(f, u)) {
                    max_gain = gain_map[{u, f}];
                    max_n = u;
                    max_tf = f;
                }
            }
        }
        if (max_n == -1) {
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
        if (gain_sum > 0 && gain_sum > max_sum) {
            max_sum = gain_sum;
            max_pass = static_cast<int>(move_seq.size() - 1);
        }
        fpgas[of].nodes.erase(max_n);
        _fpga_remove_force(of, max_n);
        nodes[max_n].fpga = max_tf;
        fpgas[max_tf].nodes.insert(max_n);
        _fpga_add_force(max_tf, max_n);
        node_state[max_n] = 2;
        active_nodes.erase(max_n);
        std::unordered_map<int, bool> node_caled;
        for (int net : nodes[max_n].nets) {
            --nets[net].fpgas[of];
            ++nets[net].fpgas[max_tf];
            for (int j = 0; j < nets[net].size; ++j) {
                int v = nets[net].nodes[j];
                if (node_state[v] == 2 || node_caled[v]) {
                    continue;
                }
                node_caled[v] = true;
                _cal_gain(v, nodes[v].fpga, sel, gain_map);
                active_nodes.insert(v);
                node_state[v] = 1;
            }
        }
    }
    // now we go back to the best solution
    for (size_t i = move_seq.size() - 1; i > max_pass; --i) {
        auto [u, of] = move_seq[i];
        int tf = nodes[u].fpga;
        fpgas[tf].nodes.erase(u);
        _fpga_remove_force(tf, u);
        nodes[u].fpga = of;
        fpgas[of].nodes.insert(u);
        _fpga_add_force(of, u);
        for (int net : nodes[u].nets) {
            ++nets[net].fpgas[of];
            --nets[net].fpgas[tf];
        }
    }
}

void HyPar::fast_k_way_localized_refine(int num, int sel) {
    std::unordered_map<int, int> node_state; // 0: inactive, 1: active, 2: locked
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes;
    for (int i = 0; i < num && !contract_memo.empty(); ++i) {
        auto [u, v] = contract_memo.top();
        int f = nodes[u].fpga;
        nodes[v].fpga = f;
        fpgas[f].nodes.insert(v);
        _uncontract(u, v, f);
        node_state[u] = 1;
        node_state[v] = 1;
        active_nodes.insert(u);
        active_nodes.insert(v);
    }
    for (int node : active_nodes) {
        _cal_gain(node, nodes[node].fpga, sel, gain_map);
    }
    int max_pass = 1.1 * num;
    for (int i = 0; i < max_pass && !gain_map.empty(); ++i) {
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        while (node_state[max_n] == 2 && !gain_map.empty()) {
            std::tie(max_gain, max_n, max_tf) = gain_map.top();
            gain_map.pop();
        }
        if (gain_map.empty()) {
            break;
        }
        gain_map.pop();
        if (max_gain <= 0) {
            break;
        }
        int of = nodes[max_n].fpga;
        fpgas[of].nodes.erase(max_n);
        _fpga_remove_force(of, max_n);
        nodes[max_n].fpga = max_tf;
        fpgas[max_tf].nodes.insert(max_n);
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
}

void HyPar::only_fast_k_way_localized_refine(int num, int sel) {
    // std::unordered_map<int, bool> node_locked;
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes;
    for (int i = 0; i < num && !contract_memo.empty(); ++i) {
        auto [u, v] = contract_memo.top();
        int f = nodes[u].fpga;
        nodes[v].fpga = f;
        fpgas[f].nodes.insert(v);
        _uncontract(u, v, f);
        active_nodes.insert(u);
        active_nodes.insert(v);
    }
    for (int node : active_nodes) {
        _cal_gain(node, nodes[node].fpga, sel, gain_map);
    }
    while(!gain_map.empty()) {
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        // while (node_locked[max_n] && !gain_map.empty()) {
        //     std::tie(max_gain, max_n, max_tf) = gain_map.top();
        //     gain_map.pop();
        // }
        // if (gain_map.empty()) {
        //     break;
        // }
        // gain_map.pop();
        if (max_gain <= 0) {
            break;
        }
        int of = nodes[max_n].fpga;
        fpgas[of].nodes.erase(max_n);
        _fpga_remove_force(of, max_n);
        nodes[max_n].fpga = max_tf;
        fpgas[max_tf].nodes.insert(max_n);
        _fpga_add_force(max_tf, max_n);
        for (int net : nodes[max_n].nets) {
            --nets[net].fpgas[of];
            ++nets[net].fpgas[max_tf];
        }
        // node_locked[max_n] = true;
        // _cal_gain(max_n, max_tf, sel, gain_map);
    }
}

bool HyPar::refine_max_connectivity() {
    std::unordered_map<int, bool> node_locked;
    std::mt19937 rng = get_rng();
    std::priority_queue<std::tuple<int, int, int>> gain_map;
    std::unordered_set<int> exceed_fpgas;
    for (int f = 0; f < K; ++f) {
        if (fpgas[f].conn > fpgas[f].maxConn) {
            exceed_fpgas.insert(f);
        }
    }
    if (exceed_fpgas.empty()) {
        return true;
    }
    std::uniform_int_distribution<int> dis(0, nets.size() - 1);
    std::unordered_map<int, bool> net_locked;
    for (int f : exceed_fpgas) {
        int netid = dis(rng);
        while (net_locked[netid]) {
            netid = dis(rng);
        }
        auto &net = nets[netid];
        if (net.fpgas[f] < net.size) {
            if (net.fpgas[f] > net.size / 2 && fpgas[f].resValid) {
                for (int i = 0; i < net.size; ++i) {
                    int u = net.nodes[i];
                    if (nodes[u].fpga != f) {
                        int uf = nodes[u].fpga;
                        _fpga_remove_force(uf, u);
                        fpgas[uf].nodes.erase(u);
                        nodes[u].fpga = f;
                        fpgas[f].nodes.insert(u);
                        _fpga_add_force(f, u);
                        for (int net : nodes[u].nets) {
                            --nets[net].fpgas[uf];
                            ++nets[net].fpgas[f];
                        }
                    }
                }
                net_locked[netid] = true;
                fpgas[f].conn -= net.weight;
            } else {
                int sf = nodes[net.source].fpga;
                if (!fpgas[sf].resValid) {
                    continue;
                }
                for (int i = 0; i < net.size; ++i) {
                    int u = net.nodes[i];
                    if (nodes[u].fpga != sf) {
                        int uf = nodes[u].fpga;
                        _fpga_remove_force(uf, u);
                        fpgas[uf].nodes.erase(u);
                        nodes[u].fpga = sf;
                        fpgas[sf].nodes.insert(u);
                        _fpga_add_force(sf, u);
                        for (int net : nodes[u].nets) {
                            --nets[net].fpgas[uf];
                            ++nets[net].fpgas[sf];
                        }
                    }
                }
                net_locked[netid] = true;
                fpgas[f].conn -= net.weight;
            }
        }
    }
    return false;
}

bool HyPar::refine_res_validity(int sel) {
    std::mt19937 rng = get_rng();
    std::priority_queue<std::tuple<int, int, int>> gain_map;
    bool flag = true;
    for (int f = 0; f < K; ++f) {
        if (fpgas[f].resValid) {
            continue;
        }
        std::priority_queue<std::tuple<int, int, int>> gain_map;
        flag = false;
        int n = 0;
        for (int i = 0; i < NUM_RES; ++i) {
            n = std::max(n, static_cast<int>((fpgas[f].resUsed[i] - fpgas[f].resCap[i]) / mean_res[i]));
        }
        std::vector<int> nodes_vec(fpgas[f].nodes.begin(), fpgas[f].nodes.end());
        std::shuffle(nodes_vec.begin(), nodes_vec.end(), rng);
        int ui = 0;
        while (!fpgas[f].resValid && !fpgas[f].nodes.empty()) {
            for(; ui < n && ui < nodes_vec.size(); ++ui) {
                int u = nodes_vec[ui];
                _cal_gain(u, f, sel, gain_map);
            }
            while (!gain_map.empty()) {
                auto [max_gain, max_n, max_tf] = gain_map.top();
                gain_map.pop();
                if (max_gain <= 0) {
                    break;
                }
                fpgas[f].nodes.erase(max_n);
                _fpga_remove_force(f, max_n);
                nodes[max_n].fpga = max_tf;
                fpgas[max_tf].nodes.insert(max_n);
                _fpga_add_force(max_tf, max_n);
                for (int net : nodes[max_n].nets) {
                    --nets[net].fpgas[f];
                    ++nets[net].fpgas[max_tf];
                }
                if (fpgas[f].resValid) {
                    break;
                }
            }
        }
    }
    return flag;
}

// this leads to endless loop!
bool HyPar::refine_max_hop() {
    bool flag = true;
    std::unordered_map<int, bool> node_locked;
    std::mt19937 rng = get_rng();
    std::vector<int> nets_vec(nets.size());
    std::iota(nets_vec.begin(), nets_vec.end(), 0);
    std::shuffle(nets_vec.begin(), nets_vec.end(), rng);
    for (int netid : nets_vec) {
        auto &net = nets[netid];
        if (net.size == 1) {
            continue;
        }
        int s = net.source;
        int sf = nodes[s].fpga;
        for (int i = 0; i < net.size; ++i) {
            int v = net.nodes[i];
            if (v == s) {
                continue;
            }
            int vf = nodes[v].fpga;
            if (fpgaMap[sf][vf] > maxHop) {
                flag = false;
                if (!node_locked[v]) {
                    std::priority_queue<std::tuple<int, int, int>> gain_map;
                    _cal_gain(v, vf, 2, gain_map);
                    if (!gain_map.empty()) {
                        node_locked[v] = true;
                        auto [_, __, tf] = gain_map.top();
                        fpgas[vf].nodes.erase(v);
                        _fpga_remove_force(vf, v);
                        nodes[v].fpga = tf;
                        fpgas[tf].nodes.insert(v);
                        _fpga_add_force(tf, v);
                        for (int net : nodes[v].nets) {
                            --nets[net].fpgas[vf];
                            ++nets[net].fpgas[tf];
                        }
                        node_locked[v] = true;
                        continue;
                    }
                } else if (!node_locked[s]) {
                    std::priority_queue<std::tuple<int, int, int>> gain_map;
                    _cal_gain(s, sf, 2, gain_map);
                    if (!gain_map.empty()) {
                        node_locked[s] = true;
                        auto [_, __, tf] = gain_map.top();
                        fpgas[sf].nodes.erase(s);
                        _fpga_remove_force(sf, s);
                        nodes[s].fpga = tf;
                        fpgas[tf].nodes.insert(s);
                        _fpga_add_force(tf, s);
                        for (int net : nodes[s].nets) {
                            --nets[net].fpgas[sf];
                            ++nets[net].fpgas[tf];
                        }
                        node_locked[s] = true;
                        break;
                    }
                }
            }
        }
    }
    return flag;
}

void HyPar::refine_random_one_node(int sel) {
    std::mt19937 rng = get_rng();
    std::vector<int> nodes_vec(existing_nodes.begin(), existing_nodes.end());
    std::shuffle(nodes_vec.begin(), nodes_vec.end(), rng);
    std::priority_queue<std::tuple<int, int, int>> gain_map;
    for (int u : nodes_vec) {
        int f = nodes[u].fpga;
        _cal_gain(u, f, sel, gain_map);
    }
}

void HyPar::force_connectivity_refine() {
    std::vector<int> fpgaVec(K);
    std::vector<double> fpgaConn(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    for (int i = 0; i < K; ++i) {
        fpgaConn[i] = double(fpgas[i].conn) / fpgas[i].maxConn;
    }
    std::sort(fpgaVec.begin(), fpgaVec.end(), [&](int a, int b) {
        return fpgaConn[a] > fpgaConn[b];
    });
    int cnt;
    for (int i = 0; i < K; ++i) {
        if (fpgaConn[fpgaVec[i]] <= 1) {
            cnt = i;
            break;
        }
    }
    for (int i = 0; i < cnt; ++i) {
        int f = fpgaVec[i];
        std::priority_queue<std::pair<int, int>> netQueue;
        for (int net = 0; net < static_cast<int>(nets.size()); ++net) {
            if (!nets[net].fpgas.count(f)) {
                continue;
            }
            if (nodes[nets[net].source].fpga == f) {
                netQueue.push({(nets[net].size - nets[net].fpgas[f]) * nets[net].weight, net});
            } else {
                netQueue.push({nets[net].fpgas[f] * nets[net].weight, net});
            }
        }
        while (fpgas[f].conn > fpgas[f].maxConn || !netQueue.empty()) {
            auto [_, net] = netQueue.top();
            netQueue.pop();
            if (fpgas[f].resValid) {
                int of;
                for (int j = 0; j < nets[net].size; ++j){
                    int v = nets[net].nodes[j];
                    if (nodes[v].fpga != f) {
                        of = nodes[v].fpga;
                        nodes[v].fpga = f;
                        _fpga_add_force(fpgaVec[cnt], v);
                        for (int net : nodes[v].nets) {
                            --nets[net].fpgas[of];
                            ++nets[net].fpgas[f];
                        }
                    }
                }
            } else {
                for (int j = K - 1; j >= cnt; --j) {
                    int tf = fpgaVec[j];
                    if (fpgas[tf].resValid) {
                        for (int k = 0; k < nets[net].size; ++k) {
                            int v = nets[net].nodes[k];
                            if (nodes[v].fpga == f) {
                                nodes[v].fpga = tf;
                                _fpga_add_force(fpgaVec[cnt], v);
                                for (int net : nodes[v].nets) {
                                    --nets[net].fpgas[f];
                                    ++nets[net].fpgas[tf];
                                }
                            }
                        }
                        break;
                    }
                }
            }
        }
    }
}

bool HyPar::force_validity_refine(int sel) {
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
        int n = 0;
        for (int i = 0; i < NUM_RES; ++i) {
            n = std::max(n, static_cast<int>((fpgas[f].resUsed[i] - fpgas[f].resCap[i]) / mean_res[i]) + 1);
        }
        std::mt19937 rng = get_rng();
        for (int i = 0; i < n; ++i) {
            std::uniform_int_distribution<int> dis(0, static_cast<int>(fpgas[f].nodes.size()) - 1);
            int step = dis(rng);
            auto it = fpgas[f].nodes.begin();
            std::advance(it, step);
            int u = *it;
            if (node_locked[u]) {
                --i;
                continue;
            }
            node_locked[u] = true;
            int max_gain = -INT_MAX, max_tf;
            for (int tf = 0; tf < K; ++tf) {
                if (tf == f || !fpgas[tf].resValid || !_fpga_add_try(tf, u)) {
                    continue;
                }
                int gain = _gain_function(f, tf, u, sel);
                if (gain > max_gain) {
                    max_gain = gain;
                    max_tf = tf;
                }
            }
            fpgas[f].nodes.erase(u);
            _fpga_remove_force(f, u);
            nodes[u].fpga = max_tf;
            fpgas[max_tf].nodes.insert(u);
            _fpga_add_force(max_tf, u);
            for (int net : nodes[u].nets) {
                ++nets[net].fpgas[max_tf];
                --nets[net].fpgas[f];
            }
        }
    }
    return false;
}

void HyPar::refine() {
    int cnt = 0;
    while (!contract_memo.empty()) {
        std::cout << "refine " << cnt++ << std::endl;
        k_way_localized_refine(2);
    }
    evaluate_summary(std::cout);
}

void HyPar::fast_refine() {
    int cnt = 0;
    while (!contract_memo.empty()) {
        k_way_localized_refine(1 + (cnt++) % 2);
    }
    evaluate_summary(std::cout);
}

void HyPar::only_fast_refine() {
    int cnt = 0;
    while (!contract_memo.empty()) {
        int num = std::log(existing_nodes.size());
        only_fast_k_way_localized_refine(num, 2);
        std::cout << "fast refine" << std::endl;
    }
    evaluate_summary(std::cout);
}