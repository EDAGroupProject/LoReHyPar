#include <hypar.hpp>
#include <algorithm>
#include <cmath>
#include <climits>
#include <cassert>

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
    std::unordered_map<int, bool> node_locked;
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
        while (node_locked[max_n] && !gain_map.empty()) {
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
        node_locked[max_n] = true;
    }
}

void HyPar::fast_refine() {
    while (!contract_memo.empty()) {
        int num = std::log(existing_nodes.size());
        std::cout << "refine num: " << num << std::endl;
        auto start = std::chrono::high_resolution_clock::now();
        fast_k_way_localized_refine(1, 1);
        auto end = std::chrono::high_resolution_clock::now();
        std::cout << "refine time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;
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

void HyPar::only_fast_refine() {
    auto start = std::chrono::high_resolution_clock::now();
    only_fast_k_way_localized_refine(contract_memo.size(), 2);
    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "refine time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();
    while (!refine_res_validity(1)) {
        continue;
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "validity time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();
    while (!refine_max_hop(1)) {
        continue;
    }
    end = std::chrono::high_resolution_clock::now();
    std::cout << "max hop time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << std::endl;
    start = std::chrono::high_resolution_clock::now();
    _fpga_cal_conn();
    while (!refine_max_connectivity(1)) {
        continue;
    }
    end = std::chrono::high_resolution_clock::now();
    // @note: how to ensure the validity of the solution?
    // Since the organizer declares that getting a valid solution is easy, we just randomly assign nodes to fpgas
}
