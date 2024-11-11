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
// the hop gain calculation is inaccurate
void HyPar::add_logic_replication(long long &hop) {
    std::vector<std::unordered_set<int>> sources(nodes.size());
    std::vector<int> net_hop(nets.size(), 0);
    std::vector<int> net_order;
    std::vector<std::vector<int>> net_fpgas(nets.size());

    for (size_t i = 0; i < nets.size(); ++i) {
        const auto &net = nets[i];
        int s = net.source;
        int sf = nodes[s].fpga;
        
        for (int u : net.nodes) {
            if (u != s) sources[u].insert(i);
        }

        int total_hop = 0;
int threshold = 2;
for (const auto &[f, cnt] : net.fpgas) {
    if (cnt > 0 && f != sf) {
        int hop_distance = fpgaMap[sf][f];
        if (hop_distance <= threshold) {
            total_hop += net.weight * std::log(hop_distance + 1);
            //total_hop += net.weight * hop_distance;
        } else {
            total_hop += net.weight * (threshold + (hop_distance - threshold) * 0.5);
        }
        net_fpgas[i].push_back(f);
    }
}





        
        net_hop[i] = total_hop;
        if (!net_fpgas[i].empty()) {
            net_order.push_back(i);
            std::sort(net_fpgas[i].begin(), net_fpgas[i].end(), [&](int a, int b) {
                return fpgaMap[sf][a] < fpgaMap[sf][b];
            });
        }
    }
    
    std::sort(net_order.begin(), net_order.end(), [&](int a, int b) {
        return net_hop[a] > net_hop[b];
    });

    while (!net_order.empty()) {
        for (auto it = net_order.begin(); it != net_order.end();) {
            int i = *it;
            auto &net = nets[i];
            int s = net.source;
            int sf = nodes[s].fpga;
            int tf = net_fpgas[i].back();
            net_fpgas[i].pop_back();

            if (net_fpgas[i].empty()) it = net_order.erase(it);
            else ++it;

            if (_fpga_add_try(tf, s)) {
                bool flag = false;
                int conn = fpgas[tf].conn - net.weight;
                int gain = net.weight * fpgaMap[sf][tf];

                for (int on : sources[s]) {
                    int osf = nodes[nets[on].source].fpga;
                    if (nets[on].fpgas[tf] == 0) {
                        if (fpgaMap[osf][tf] > maxHop) { flag = true; break; }
                        gain -= nets[on].weight * fpgaMap[osf][tf];
                        conn += nets[on].weight;
                    }
                    if (nets[on].fpgas[osf] == nets[on].size && osf != tf) {
                        if (fpgas[osf].conn + nets[on].weight > fpgas[osf].maxConn) {
                            flag = true;
                            break;
                        }
                    }
                }

                if (!flag && conn <= fpgas[tf].maxConn && gain > 0) {
                    hop -= gain;
                    int new_s = nodes.size();
                    nodes.push_back(Node{ nodes[s].name + "*", s, tf });
                    fpgas[tf].nodes.insert(new_s);
                    _fpga_add_force(tf, s);
                    fpgas[tf].conn = conn;
                    
                    net.size -= net.fpgas[tf];
                    net.fpgas[tf] = 0;

                    for (int on : sources[s]) {
                        nets[on].nodes.push_back(new_s);
                        nets[on].size++;
                        nets[on].fpgas[tf]++;
                        int osf = nodes[nets[on].source].fpga;
                        if (nets[on].fpgas[osf] == nets[on].size - 1) {
                            fpgas[osf].conn += nets[on].weight;
                        }
                    }
                }
            }
        }
    }
}
