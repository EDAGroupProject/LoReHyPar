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
void HyPar::k_way_localized_refine() {
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
    _cal_refine_gain(u, f, gain_map);
    _cal_refine_gain(v, f, gain_map);
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
        for (int net : nodes[max_n].nets) {
            --nets[net].fpgas[of];
            ++nets[net].fpgas[max_tf];
            for (int j = 0; j < nets[net].size; ++j) {
                int node = nets[net].nodes[j];
                if (node_state[node] == 0) {
                    node_state[node] = 1;
                    active_nodes.insert(node);
                    _cal_refine_gain(node, nodes[node].fpga, gain_map);
                } else if (node_state[node] == 1) { // @warning: Currently, we do not consider the case when the move enlarges the choice set
                    if (nets[net].source == max_n || nets[net].source == node) {
                        for (int f = 0; f < K; ++f) {
                            if (gain_map.count({node, f})) {
                                gain_map[{node, f}] += fpgaMap[max_tf][nodes[node].fpga] - fpgaMap[max_tf][K] - fpgaMap[of][nodes[node].fpga] + fpgaMap[of][K];
                            }
                        }
                    }
                }
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

void HyPar::random_validity_refine() {
    std::vector<int> fpgaVec(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), get_rng());
    std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map;
    for (int f : fpgaVec) {
        if (!fpgas[f].resValid) {
            std::vector<int> nodeVec(fpgas[f].nodes);
            std::shuffle(nodeVec.begin(), nodeVec.end(), get_rng());
            for (int node : nodeVec) {
                std::mt19937 rng = get_rng();
                std::uniform_int_distribution<int> dist(0, K - 1);
                int ff = dist(rng);
                while (ff == f) {
                    ff = dist(rng);
                }
                fpgas[f].nodes.erase(std::remove(fpgas[f].nodes.begin(), fpgas[f].nodes.end(), node), fpgas[f].nodes.end());
                fpgas[ff].nodes.push_back(node);
                nodes[node].fpga = ff;
                _fpga_remove_force(f, node);
                _fpga_add_force(ff, node);
                for (int net : nodes[node].nets) {
                    --nets[net].fpgas[f];
                    ++nets[net].fpgas[ff];
                    if (nets[net].fpgas[f] == 0) {
                        nets[net].fpgas.erase(f);
                        if (nets[net].fpgas.size() > 1) {
                            fpgas[f].conn -= nets[net].weight;
                        }
                    }
                    if (nets[net].fpgas[ff] == 1 && nets[net].fpgas.size() > 1) {
                        fpgas[ff].conn += nets[net].weight;
                    }
                }
                if (fpgas[f].resValid) {
                    break;
                }
            }
        }
    }
}

void HyPar::refine() {
    while (!contract_memo.empty()) {
        k_way_localized_refine();
    }
    _fpga_cal_conn();
    // @note: how to ensure the validity of the solution?
    // Since the organizer declares that getting a valid solution is easy, we just randomly assign nodes to fpgas
    random_validity_refine();
}