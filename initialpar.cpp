#include <hypar.hpp>
#include <climits>
#include <algorithm>

void HyPar::bfs_partition() {
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

void HyPar::SCLa_propagation() {
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
void HyPar::greedy_hypergraph_growth(int sel) { // I use this based on the former initial partition results
    int gain_sum = 0, cnt = 0, min_pass = std::log(nodes.size());
    int max_sum = -INT_MAX, max_pass = -1;
    int total_gain_sum = 0;
    while (cnt < min_pass){ // the end condition can be modified
        total_gain_sum += max_sum;
        max_sum = -INT_MAX, max_pass = -1;
        gain_sum = 0;
        std::vector<std::pair<int,int>> move_seq; // (node, from_fpga)
        std::vector<int> node_state(nodes.size(), 1); // 0: active, 1: locked
        std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map; // (node, fpga) -> gain
        std::unordered_set<int> active_nodes(existing_nodes);
        for (int node : active_nodes) {
            _cal_inpar_gain(node, nodes[node].fpga, sel, gain_map);
        }
        while (!active_nodes.empty()) {
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
            active_nodes.erase(max_n);
            node_state[max_n] = 1;
            for (int net : nodes[max_n].nets){
                --nets[net].fpgas[of];
                ++nets[net].fpgas[max_tf];
                for (int j = 0; j < nets[net].size; ++j) {
                    int v = nets[net].nodes[j];
                    if (node_state[v] == 0) { // active
                        _cal_inpar_gain(v, nodes[v].fpga, sel, gain_map);
                    }
                }
            }
        }
        ++cnt;
        if (max_pass == -1) {
            break;
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
        if (max_sum < double(total_gain_sum) / (cnt - 1)) {
            break;
        }
    }
}

// @todo: other partitioning methods to enrich the portfolio
void HyPar::initial_partition() {
    bool bestvalid = false, valid;
    long long minHop = LONG_LONG_MAX, hop;
    HyPar best, tmp;
    for (int i = 0; i < 10; ++i){
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
    for (int i = 0; i < 10; ++i){
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
    // while (flag) {
    //     flag = false;
    //     for (int i = 1; i < 4; ++i){
    //         tmp.greedy_hypergraph_growth(i);
    //         tmp._fpga_cal_conn();
    //         tmp.evaluate(valid, hop);
    //         if ((!bestvalid || valid) && hop < minHop) {
    //             flag = true;
    //             best = tmp;
    //             bestvalid = valid;
    //             minHop = hop;
    //         }
    //         std::cout << "Greedy Hypergraph Growth: " << hop << std::endl;
    //     }
    // }
    *this = std::move(best);
    std::cout << "Initial Partition: " << hop << std::endl;
}

// how to meet the connectivity constraint?
// @todo: use connectivity metric to initialize the partition
// for a net, its connectivity equals to the size of the unordered_map fpgas

// void HyPar::map_fpga() {
//     // @todo: mapping fpga to the coarsest partition
//     // maybe greedy algorithm is enough
//     // currently, we just randomly assign nodes to fpgas
// }