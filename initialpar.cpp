#include <hypar.hpp>

void HyPar::SCLa_propagation() {
    std::mt19937 rng = get_rng();
    std::vector<int> fpgaVec(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), rng);
    std::vector<int> unlabeled_nodes(existing_nodes.begin(), existing_nodes.end());
    std::shuffle(unlabeled_nodes.begin(), unlabeled_nodes.end(), rng);
    for (int i = 0; i < K; ++i) {
        int f = fpgaVec[i];
        int id = (unlabeled_nodes.size() * i) / K;
        int u = unlabeled_nodes[id];
        unlabeled_nodes.erase(unlabeled_nodes.begin() + id);
        nodes[u].fpga = f;
        fpgas[f].nodes.insert(u);
        _fpga_add_force(f, u);
        int cnt = 0;
        for (int net : nodes[u].nets) {
            ++nets[net].fpgas[f];
            if (cnt >= parameter_tau) {
                continue;
            }
            std::uniform_int_distribution<int> dis(0, nets[net].size - 1);
            auto it = nets[net].nodes.begin();
            std::advance(it, dis(rng));
            int v = *it;
            if (nodes[v].fpga == -1 && _fpga_add_try(f, v)) {
                ++cnt;
                unlabeled_nodes.erase(std::find(unlabeled_nodes.begin(), unlabeled_nodes.end(), v));
                _fpga_add_force(f, v);
                nodes[v].fpga = f;
                fpgas[f].nodes.insert(v);
                for (int vnet : nodes[v].nets) {
                    ++nets[vnet].fpgas[f];
                }
            }
        }
    }
    for (int u : unlabeled_nodes) {
        std::shuffle(fpgaVec.begin(), fpgaVec.end(), rng);
        std::unordered_set<int> toFpga(fpgaVec.begin(), fpgaVec.end());
        for (int tf : fpgaVec) {
            if (!_fpga_add_try(tf, u)) {
                toFpga.erase(tf);
            }
        }
        std::vector<int> tfs(K, 0);
        for (int net : nodes[u].nets) {
            int s = nets[net].source;
            if (u == s) {
                for (const auto &[f, cnt] : nets[net].fpgas) {
                    if (cnt) {
                        for (auto it = toFpga.begin(); it != toFpga.end();) {
                            int tf = *it;
                            tfs[tf] += nets[net].size;
                            if (fpgaMap[f][tf] > maxHop) {
                                it = toFpga.erase(it);
                            } else {
                                ++it;
                            }
                        }
                    }
                }
            } else {
                int sf = nodes[s].fpga;
                if (sf == -1) {
                    continue;
                }
                for (auto it = toFpga.begin(); it != toFpga.end();) {
                    int tf = *it;
                    tfs[tf] += nets[net].size;
                    if (fpgaMap[sf][tf] > maxHop) {
                        it = toFpga.erase(it);
                    } else {
                        ++it;
                    }
                }
            }
        }
        if (toFpga.empty()) {
            int max_tf = std::max_element(tfs.begin(), tfs.end()) - tfs.begin();
            nodes[u].fpga = max_tf;
            fpgas[max_tf].nodes.insert(u);
            _fpga_add_force(max_tf, u);
            for (int net : nodes[u].nets) {
                ++nets[net].fpgas[max_tf];
            }
            continue;
        }
        int max_tf = -1, max_gain = -INT_MAX;
        for (int tf : toFpga) {
            if (tfs[tf] > max_gain) {
                max_gain = tfs[tf];
                max_tf = tf;
            }
        }
        nodes[u].fpga = max_tf;
        fpgas[max_tf].nodes.insert(u);
        _fpga_add_force(max_tf, u);
        for (int net : nodes[u].nets) {
            ++nets[net].fpgas[max_tf];
        }
    }
}

void HyPar::greedy_hypergraph_growth(int sel) { // I use this based on the former initial partition results
    std::vector<std::pair<int,int>> move_seq; // (node, from_fpga)
    std::unordered_map<int, int> node_state; // 0: inactive, 1: active, 2: locked
    std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map; // (node, fpga) -> gain
    std::unordered_set<int> active_nodes;
    std::vector<int> random_steps(K);
    std::mt19937 rng = get_rng();
    std::uniform_int_distribution<int> dis(0, existing_nodes.size() - 1);
    for (int i = 0; i < K; ++i) {
        int step = dis(rng);
        while (std::find(random_steps.begin(), random_steps.end(), step) != random_steps.end()) {
            step = dis(rng);
        }
        random_steps[i] = step;
    }
    for (int i = 0; i < K; ++i) {
        auto it = existing_nodes.begin();
        std::advance(it, random_steps[i]);
        int u = *it;
        node_state[u] = 1;
        active_nodes.insert(u);
        _cal_gain(u, nodes[u].fpga, sel, gain_map);
    }
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

void HyPar::activate_max_hop_nodes(int sel) {
    std::priority_queue<std::tuple<int, int, int>> gain_map; // (gain, node, fpga)
    std::unordered_set<int> active_nodes;
    for (const auto &net : nets) {
        if (net.size == 1) {
            continue;
        }
        int s = net.source;
        int sf = nodes[s].fpga;
        for (int i = 0; i < net.size; ++i) {
            int u = net.nodes[i];
            int uf = nodes[u].fpga;
            if (fpgaMap[sf][uf] > maxHop) {
                active_nodes.insert(u);
                active_nodes.insert(s);
            }
        }
    }
    if (active_nodes.empty()) {
        return;
    }
    auto it = active_nodes.begin();
    int size = active_nodes.size();
    for (int i = 0; i < size; ++i) {
        int u = *it;
        for (int net : nodes[u].nets) {
            for (int i = 0; i < nets[net].size; ++i) {
                int v = nets[net].nodes[i];
                active_nodes.insert(v);
            }
        }
        ++it;
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

void HyPar::initial_partition() {
    bool bestvalid = false, valid;
    long long minHop = LONG_LONG_MAX, hop;
    HyPar best, tmp;
    for (int i = 0; i < 20; ++i){
        tmp = *this;
        tmp.SCLa_propagation();
        tmp.evaluate(valid, hop);
        if (bestvalid <= valid && hop < minHop) {
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
    }
    *this = std::move(best);
    bool flag = true;
    while (flag) {
        flag = false;
        greedy_hypergraph_growth(1);
        evaluate(valid, hop);
        if (bestvalid <= valid && hop < minHop) {
            flag = true;
            bestvalid = valid;
            minHop = hop;
        }
    }
    activate_max_hop_nodes(0);
}

void HyPar::fast_initial_partition() {
    bool valid, bestvalid = false;
    long long hop, minHop = LONG_LONG_MAX;
    SCLa_propagation();
    bool flag = true;
    while (flag) {
        flag = false;
        greedy_hypergraph_growth(1);
        evaluate(valid, hop);
        if (bestvalid <= valid && hop < minHop) {
            flag = true;
            bestvalid = valid;
            minHop = hop;
        }
    }
    activate_max_hop_nodes(0);
}
