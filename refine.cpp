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
    nodes[v].fpga = f;
    fpgas[f].nodes.insert(v);
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
    // now we go back to the best solution
    for (int i = static_cast<int>(move_seq.size()) - 1; i > max_pass; --i) {
        auto [node, of] = move_seq[i];
        int tf = nodes[node].fpga;
        fpgas[tf].nodes.erase(node);
        _fpga_remove_force(tf, node);
        nodes[node].fpga = of;
        fpgas[of].nodes.insert(node);
        _fpga_add_force(of, node);
        for (int net : nodes[node].nets) {
            --nets[net].fpgas[tf];
            ++nets[net].fpgas[of];
        }
    }
}



bool HyPar::refine_max_connectivity(int sel) {
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
            if (net.fpgas[f] > net.size / 2) {
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
                fpgas[f].conn -= net.weight;
            } else {
                int sf = nodes[net.source].fpga;
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
                fpgas[f].conn -= net.weight;
            }
        }
    }
    return false;
}

bool HyPar::refine_res_validity(int sel) {
    std::unordered_map<int, bool> node_locked;
    std::mt19937 rng = get_rng();
    std::priority_queue<std::tuple<int, int, int>> gain_map;
    for (int f = 0; f < K; ++f) {
        if (fpgas[f].resValid) {
            continue;
        }
        std::uniform_int_distribution<int> dis(0, fpgas[f].nodes.size() - 1);
        auto it = fpgas[f].nodes.begin();
        std::advance(it, dis(rng));
        int s = *it;
        int sf = nodes[s].fpga;
        _cal_gain(s, sf, sel, gain_map);
    }
    if (gain_map.empty()) {
        return true;
    }
    while (!gain_map.empty()) {
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        while (node_locked[max_n] && !gain_map.empty()) {
            std::tie(max_gain, max_n, max_tf) = gain_map.top();
            gain_map.pop();
        }
        if (gain_map.empty()) {
            break;
        }
        node_locked[max_n] = true;
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
    }
    return false;
}

bool HyPar::refine_max_hop(int sel) {
    bool flag = true;
    std::unordered_map<int, bool> node_locked;
    for (const auto &net : nets) {
        if (net.size == 1) {
            continue;
        }
        std::unordered_set<int> active_nodes;
        int s = net.source;
        int sf = nodes[s].fpga;

        // Debugging output to check the value of s and sf
        std::cout << "Processing net with source node: " << s << ", sf: " << sf << std::endl;

        for (int i = 0; i < net.size; ++i) {
            int v = net.nodes[i];
            if (v == s) {
                continue;
            }
            int vf = nodes[v].fpga;

            // Debugging output to check the values of v and vf
            std::cout << "Processing node: " << v << ", vf: " << vf << std::endl;

            // Boundary check to ensure sf and vf are within the valid range
            if (sf < 0 || sf >= fpgaMap.size() || vf < 0 || vf >= fpgaMap[sf].size()) {
                std::cerr << "Error: sf or vf out of bounds. sf: " << sf << ", vf: " << vf << std::endl;
                continue;
            }

            if (fpgaMap[sf][vf] > maxHop) {
                flag = false;
                if (!node_locked[s]) {
                    active_nodes.insert(s);
                }
                if (!node_locked[v]) {
                    active_nodes.insert(v);
                }
            }
        }

        if (active_nodes.empty()) {
            continue;
        }
        std::priority_queue<std::tuple<int, int, int>> gain_map;
        for (int node : active_nodes) {
            _cal_gain(node, nodes[node].fpga, sel, gain_map);
        }
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        if (max_n == s) {
            node_locked[s] = true;
            fpgas[sf].nodes.erase(s);
            _fpga_remove_force(sf, s);
            nodes[s].fpga = max_tf;
            fpgas[max_tf].nodes.insert(s);
            _fpga_add_force(max_tf, s);
            for (int net : nodes[s].nets) {
                --nets[net].fpgas[sf];
                ++nets[net].fpgas[max_tf];
            }
        } else {
            while (!gain_map.empty()) {
                auto [max_gain, max_n, max_tf] = gain_map.top();
                gain_map.pop();
                if (max_n == s || max_gain <= 0) {
                    break;
                }
                node_locked[max_n] = true;
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
                if (max_n == s) {
                    break;
                }
            }
        }
    }
    return flag;
}

void HyPar::refine() {
    while (!contract_memo.empty()) {
        auto start = std::chrono::high_resolution_clock::now();
        k_way_localized_refine(2);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "refine time: " << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count() << std::endl;
    }
    while (!refine_res_validity(1)) {
        continue;
    }
    while (!refine_max_hop(1)) {
        continue;
    }
    _fpga_cal_conn();
    while (!refine_max_connectivity(1)) {
        continue;
    }
    // @note: how to ensure the validity of the solution?
    // Since the organizer declares that getting a valid solution is easy, we just randomly assign nodes to fpgas
}

