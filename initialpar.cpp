#include <hypar.hpp>
#include <algorithm>

void ParFunc::bfs_partition(){
    std::vector<int> nodeVec(curNodes.begin(), curNodes.end());
    std::shuffle(nodeVec.begin(), nodeVec.end(), global_rng);
    std::vector<int> fpgaVec(K);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), global_rng);
    std::uniform_int_distribution<int> dis(0, K - 1);
    for (auto it = nodeVec.begin(); it != nodeVec.end();){
        if (nodes[*it].fpga != -1){
            it = nodeVec.erase(it);
            continue;
        } 
        int cur = dis(global_rng);
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
            int cur = dis(global_rng); // @warning: it's too arbitrary
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

void ParFunc::initial_partition(){
    bfs_partition();
    _fpga_cal_conn();
}

// how to meet the connectivity constraint?
// @todo: use connectivity metric to initialize the partition
// for a net, its connectivity equals to the size of the unordered_map fpgas

// void ParFunc::map_fpga(){
//     // @todo: mapping fpga to the coarsest partition
//     // maybe greedy algorithm is enough
//     // currently, we just randomly assign nodes to fpgas
// }