#include <hypar.hpp>

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
    for (int i = static_cast<int>(move_seq.size()) - 1; i > max_pass; --i) {
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

// only refine uncontracted nodes and their neighbors
void HyPar::fast_k_way_localized_refine(int num, int sel) {
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes;
    for (int i = 0; i < num && !contract_memo.empty(); ++i) {
        auto [u, v] = contract_memo.top();
        int f = nodes[u].fpga;
        _uncontract(u, v, f);
        active_nodes.insert(u);
        active_nodes.insert(v);
        for (int net : nodes[u].nets) {
            for (int j = 0; j < nets[net].size; ++j) {
                int w = nets[net].nodes[j];
                active_nodes.insert(w);
            }
        }
        for (int net : nodes[v].nets) {
            for (int j = 0; j < nets[net].size; ++j) {
                int w = nets[net].nodes[j];
                active_nodes.insert(w);
            }
        }
    }
    for (int node : active_nodes) {
        _cal_gain(node, nodes[node].fpga, sel, gain_map);
    }
    while(!gain_map.empty()) {
        auto [max_gain, max_n, max_tf] = gain_map.top();
        gain_map.pop();
        while (active_nodes.count(max_n) && !gain_map.empty()) {
            std::tie(max_gain, max_n, max_tf) = gain_map.top();
            gain_map.pop();
        }
        if (gain_map.empty()) {
            break;
        }
        if (max_gain < 0) {
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
    }
}

// only refine uncontracted nodes themselves
void HyPar::only_fast_k_way_localized_refine(int num, int sel) {
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes;
    for (int i = 0; i < num && !contract_memo.empty(); ++i) {
        auto [u, v] = contract_memo.top();
        int f = nodes[u].fpga;
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
        while (active_nodes.count(max_n) && !gain_map.empty()) {
            std::tie(max_gain, max_n, max_tf) = gain_map.top();
            gain_map.pop();
        }
        if (gain_map.empty()) {
            break;
        }
        if (max_gain < 0) {
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
    }
}

void HyPar::refine() {
    while (!contract_memo.empty()) {
        k_way_localized_refine(0);
    }
}

void HyPar::fast_refine() {
    while (!contract_memo.empty()) {
        int num = std::sqrt(contract_memo.size());
        fast_k_way_localized_refine(num, 0);
    }
}

void HyPar::only_fast_refine() {
    while (!contract_memo.empty()) {
        int num = std::sqrt(contract_memo.size());
        only_fast_k_way_localized_refine(num, 0);
    }
}

// didn't consider the of the influence of the formerly replicated nodes on the gain calculation
void HyPar::add_logic_replication() {
    std::vector<std::unordered_set<int>> sources(N);
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    for (size_t i = 0; i < nets.size(); ++i) {
        const auto &net = nets[i];
        int s = net.source;
        for (int j = 0; j < net.size; ++j) {
            int u = net.nodes[j];
            if (u == s) {
                continue;
            }
            sources[u].insert(i);
        }
    }
    for (const auto &net : nets) {
        int s = net.source;
        int sf = nodes[s].fpga;
        for (const auto &[f, cnt] : net.fpgas) {
            if (cnt == 0 || f == sf || fpgaMap[sf][f] > maxHop || !_fpga_add_try(f, s)) {
                continue;
            }
            int gain = 0;
            bool flag = false;
            int conn = fpgas[f].conn;
            // worst estimation on connectivity
            for (int on : sources[s]) {
                if (nets[on].fpgas[f] == 0) {
                    int osf = nodes[nets[on].source].fpga;
                    if (fpgaMap[osf][f] > maxHop) {
                        flag = true;
                        break;
                    }
                    gain -= nets[on].weight * fpgaMap[osf][f];
                }
                conn += nets[on].weight;
            }
            if (flag) {
                continue;
            }
            if (conn > fpgas[f].maxConn) {
                continue;
            }
            gain += net.weight * fpgaMap[sf][f];
            gain_map.push({gain, s, f});
        }
    }
    while (!gain_map.empty()) {
        auto [gain, u, f] = gain_map.top();
        gain_map.pop();
        if (gain < 0) {
            break;
        }
        if (!_fpga_add_try(f, u)) {
            continue;
        }
        nodes.emplace_back();
        int new_u = nodes.size() - 1;
        nodes[new_u] = nodes[u];
        nodes[new_u].rep = u;
        nodes[new_u].fpga = f;
        fpgas[f].nodes.insert(new_u);
        _fpga_add_force(f, new_u);
    }
}