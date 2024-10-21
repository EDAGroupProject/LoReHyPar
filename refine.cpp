#include <hypar.hpp>
#include <climits>

void ParFunc::refine_naive(){
    // @warning: the following code is just uncontract all nodes
    while (!contMeme.empty()){
        auto [u, v] = contMeme.top();
        _uncontract(u, v);
        int f = nodes[u].fpga;
        nodes[v].fpga = f;
        fpgas[f].nodes.push_back(v);
    }
}

void ParFunc::k_way_localized_refine(){
    auto [u, v] = contMeme.top();
    _uncontract(u, v);
    int f = nodes[u].fpga;
    nodes[v].fpga = f;
    fpgas[f].nodes.push_back(v);
    std::vector<std::tuple<int,int,int>> move_seq; // (gain, node, from_fpga)
    std::vector<int> node_state(nodes.size(), 0); // 0: inactive, 1: active, 2: locked
    std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map; // (node, fpga) -> gain
    std::unordered_set<int> active_nodes;
    active_nodes.insert(u);
    active_nodes.insert(v);
    _cal_refine_gain(u, f, gain_map);
    _cal_refine_gain(v, f, gain_map);
    int gain_sum = 0, cnt = 0, min_pass = log(nodes.size());
    int max_sum = -INT_MAX, max_pass = 0;
    while (cnt < min_pass || gain_sum > 0){
        int max_gain = -INT_MAX, max_n, max_f;
        for (int node : active_nodes){
            for (int f = 0; f < K; ++f){
                if (fpgas[f].valid && gain_map[{node, f}] > max_gain){
                    max_gain = gain_map[{node, f}];
                    max_n = node;
                    max_f = f;
                }
            }
        }
        int of = nodes[max_n].fpga;
        move_seq.emplace_back(max_gain, max_n, of);
        gain_sum += max_gain;
        if (gain_sum > max_sum){
            max_sum = gain_sum;
            max_pass = cnt++;
        }
        fpgas[of].nodes.erase(std::remove(fpgas[of].nodes.begin(), fpgas[of].nodes.end(), max_n), fpgas[of].nodes.end());
        nodes[max_n].fpga = max_f;
        fpgas[max_f].nodes.push_back(max_n);
        fpgas[max_f].valid = _fpga_add_res_cal_conn_force(max_f, max_n);
        node_state[max_n] = 2;
        active_nodes.erase(max_n);
        for (int net : nodes[max_n].nets){
            for (int node : nets[net].nodes){
                if (node_state[node] == 0){
                    node_state[node] = 1;
                    active_nodes.insert(node);
                    _cal_refine_gain(node, nodes[node].fpga, gain_map);
                } else if (node_state[node] == 1){
                    // @todo: incrementally update the gain
                    // @todo: discard internel nodes
                    // _cal_refine_gain(node, nodes[node].fpga, gain_map);
                    // @note: maybe we can ...
                    if (nets[net].source == max_n || nets[net].source == node){
                        for (int f = 0; f < K; ++f){
                            if (gain_map[{node, f}].count()){
                                gain_map[{node, f}] += fpgaMap[max_f][nodes[node].fpga] - fpgaMap[max_f][K] - fpgaMap[of][nodes[node].fpga] + fpgaMap[of][K];
                            }
                        }
                    }
                }
            }
        }
    }
}

void ParFunc::random_validity_refine(){
    std::vector<int> fpgaVec(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), global_rng);
    std::unordered_map<std::pair<int,int>, int, pair_hash> gain_map;
    for (int f : fpgaVec){
        if (!fpgas[f].valid){
            std::vector<int> nodeVec(fpgas[f].nodes);
            std::shuffle(nodeVec.begin(), nodeVec.end(), global_rng);
            for (int node : nodeVec){
                int ff = uniform_int_distribution<int>(0, K - 1)(global_rng);
                while (ff == f){
                    ff = uniform_int_distribution<int>(0, K - 1)(global_rng);
                }
                fpgas[f].nodes.erase(std::remove(fpgas[f].nodes.begin(), fpgas[f].nodes.end(), node), fpgas[f].nodes.end());
                fpgas[ff].nodes.push_back(node);
                nodes[node].fpga = ff;
                fpgas[f].valid = _fpga_remove_force(f, node);
                fpgas[ff].valid = _fpga_add_force(max_f, node);
                if (fpgas[f].valid){
                    break;
                }
            }
        }
    }
}

void ParFunc::refine(){
    // for (auto &fpga : fpgas){
    //     fpga.query_validity();
    // }
    while (!contMeme.empty()){
        k_way_localized_refine();
    }
    random_validity_refine();
}