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
        fpgas[f].nodes.push_back(v);
    }
}

// @warning: Currently, we do not consider the case when the move enlarges the choice set
// so we need to calculate the gain of the move each time we call this function
// if we can take this into account, we will not need to calculate the gain each time
// @todo: use different gain function to improve both hop and connectivity
void HyPar::k_way_localized_refine(int sel) {
    auto [u, v] = contract_memo.top();
    _uncontract(u, v);
    int f = nodes[u].fpga;
    nodes[v].fpga = f;
    fpgas[f].nodes.push_back(v);
    for (int net : nodes[v].nets) {
        ++nets[net].fpgas[f];
    }
    std::vector<std::pair<int,int>> move_seq; // (node, from_fpga)
    std::unordered_map<int, int> node_state; // 0: inactive, 1: active, 2: locked
    std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map; // (node, fpga) -> gain
    std::unordered_set<int> active_nodes;
    node_state[u] = 1;
    node_state[v] = 1;
    active_nodes.insert(u);
    active_nodes.insert(v);
    _cal_refine_gain(u, f, sel, gain_map);
    _cal_refine_gain(v, f, sel, gain_map);
    int gain_sum = 0, cnt = 0, min_pass = std::log(nodes.size()) / std::log(2);
    int max_sum = -INT_MAX, max_pass = 0;
    while (cnt < min_pass || gain_sum > 0) {
        int max_gain = -INT_MAX, max_n = -1, max_tf;
        for (int node : active_nodes) {
            for (int f = 0; f < K; ++f) {
                if (fpgas[f].resValid && gain_map.count({node, f}) && gain_map[{node, f}] > max_gain) {
                    max_gain = gain_map[{node, f}];
                    max_n = node;
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
                _cal_refine_gain(v, nodes[v].fpga, sel, gain_map);
                node_state[v] = 1;
                // if (node_state[node] == 0) {
                //     node_state[node] = 1;
                //     active_nodes.insert(node);
                //     _cal_refine_gain(node, nodes[node].fpga, sel, gain_map);
                // } else if (node_state[node] == 1) { // @warning: Currently, we do not consider the case when the move enlarges the choice set
                //     if (nets[net].source == max_n || nets[net].source == node) {
                //         for (int f = 0; f < K; ++f) {
                //             if (gain_map.count({node, f})) {
                //                 gain_map[{node, f}] += fpgaMap[max_tf][nodes[node].fpga] - fpgaMap[max_tf][K] - fpgaMap[of][nodes[node].fpga] + fpgaMap[of][K];
                //             }
                //         }
                //     }
                // }
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
    int cnt = 0;
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

void HyPar::refine() {
    int sel[2] = {1, 2}, cnt = 0;
    while (!contract_memo.empty()) {
        k_way_localized_refine(sel[(cnt++) % 2]);
        // if (_fpga_cal_conn(), !_fpga_chk_conn()) {
        //     force_connectivity_refine();
        // }
    }
    if (!_fpga_chk_res()) {
        for (int i = 0; i < K; ++i) {
            if(force_validity_refine(2)) {
                break;
            }
        }
    }
    // @note: how to ensure the validity of the solution?
    // Since the organizer declares that getting a valid solution is easy, we just randomly assign nodes to fpgas
}