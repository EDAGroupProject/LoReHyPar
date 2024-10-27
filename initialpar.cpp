#include <hypar.hpp>
#include <climits>
#include <algorithm>

void HyPar::bfs_partition(){
    std::vector<int> nodeVec(existing_nodes.begin(), existing_nodes.end());
    std::mt19937 rng = get_rng();
    std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
    std::vector<int> fpgaVec(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), rng);
    std::uniform_int_distribution<int> dis(0, K - 1);
    for (auto it = nodeVec.begin(); it != nodeVec.end();){
        if (nodes[*it].fpga != -1){
            it = nodeVec.erase(it);
            continue;
        } 
        int cur = dis(rng);
        int f = fpgaVec[cur];
        if (_fpga_add_try(f, *it)){
            std::queue<int> q;
            q.push(*it);
            while (!q.empty()){
                int u = q.front();
                q.pop();
                nodes[u].fpga = f;
                fpgas[f].nodes.push_back(u);
                for (int net : nodes[u].nets){
                    ++nets[net].fpgas[f]; // ready for the connectivity calculation
                    for (int i = 0; i < nets[net].size; ++i){
                        int v = nets[net].nodes[i];
                        if (nodes[v].fpga == -1 && _fpga_add_try(f, v)){
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
    if (!nodeVec.empty()){
        for(auto node : nodeVec){
            int cur = dis(rng); // @warning: it's too arbitrary
            int f = fpgaVec[cur];
            nodes[node].fpga = f;
            fpgas[f].nodes.push_back(node);
            _fpga_add_force(f, node);
            for (int net : nodes[node].nets){
                ++nets[net].fpgas[f];
            }
        }
    }
}

void HyPar::SCLa_propagation(){
    std::unordered_set<int> unlabeled_nodes(existing_nodes);
    std::vector<int> nodeVec(existing_nodes.begin(), existing_nodes.end());
    std::mt19937 rng = get_rng();
    std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
    int step = nodeVec.size() / K;
    for (int i = 0; i < K; ++i){
        int u = nodeVec[i * step];
        unlabeled_nodes.erase(u);
        nodes[u].fpga = i;
        fpgas[i].nodes.push_back(u);
        _fpga_add_force(i, u);
        int cnt = 0;
        for (int net : nodes[u].nets){
            ++nets[net].fpgas[i];
            if (cnt >= parameter_tau){
                continue;
            }
            for (int j = 0; j < nets[net].size; ++j){
                int v = nets[net].nodes[j];
                if (cnt >= parameter_tau){
                    break;
                }
                if (nodes[v].fpga == -1 && _fpga_add_try(i, v)){
                    ++cnt;
                    unlabeled_nodes.erase(v);
                    nodes[v].fpga = i;
                    fpgas[i].nodes.push_back(v);
                    for (int vnet : nodes[v].nets){
                        ++nets[vnet].fpgas[i];
                    }
                }
            }
        }
    }
    for (int u : unlabeled_nodes){
        std::unordered_map<int, int> gain_map;
        for (int net : nodes[u].nets){
            for (auto [f, cnt] : nets[net].fpgas){
                if (cnt == 0){
                    continue;
                }
                gain_map[f] += cnt * nets[net].weight; // @warning: the gain is arbitrary
            }
        }
        int f_max = -1, gain_max = 0;
        for (auto [f, g] : gain_map){
            if (g > gain_max && _fpga_add_try_nochange(f, u)){
                f_max = f;
                gain_max = g;
            }
        }
        if (f_max == -1){
            std::uniform_int_distribution<int> dis(0, gain_map.size() - 1);
            bool steps = dis(rng);
            auto it = gain_map.begin();
            advance(it, steps);
            f_max = it->first;
        }
        nodes[u].fpga = f_max;
        fpgas[f_max].nodes.push_back(u);
        _fpga_add_force(f_max, u);
        for (int net : nodes[u].nets){
            ++nets[net].fpgas[f_max];
        }
    }
}

// @todo: other partitioning methods to enrich the portfolio
void HyPar::initial_partition(){
    bool bestvalid = false, valid;
    int minHop = INT_MAX, hop;
    HyPar best, tmp;
    for (int i = 0; i < 20; ++i){
        tmp = *this;
        tmp.bfs_partition();
        tmp._fpga_cal_conn();
        auto [valid, hop] = tmp.evaluate();
        if ((!bestvalid || valid) && hop < minHop){
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
        tmp = *this;
        tmp.SCLa_propagation();
        tmp._fpga_cal_conn();
        auto [valid, hop] = tmp.evaluate();
        if ((!bestvalid || valid) && hop < minHop){
            best = tmp;
            bestvalid = valid;
            minHop = hop;
        }
    }
}

// how to meet the connectivity constraint?
// @todo: use connectivity metric to initialize the partition
// for a net, its connectivity equals to the size of the unordered_map fpgas

// void HyPar::map_fpga(){
//     // @todo: mapping fpga to the coarsest partition
//     // maybe greedy algorithm is enough
//     // currently, we just randomly assign nodes to fpgas
// }