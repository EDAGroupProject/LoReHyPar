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
        for (int i = 0; i < K; ++i){
            if (_fpga_add_res(fpgaVec[cur], *it)){
                nodes[*it].fpga = fpgaVec[cur];
                fpgas[fpgaVec[cur]].nodes.push_back(*it);
                std::queue<int> q;
                q.push(*it);
                while (!q.empty()){
                    int u = q.front();
                    q.pop();
                    for (int net : nodes[u].nets){
                        for (size_t j = 0; j < nets[net].size; ++j){
                            int v = nets[net].nodes[j];
                            if (nodes[v].fpga == -1 && _fpga_add_res(fpgaVec[cur], v) && _fpga_cal_conn(fpgaVec[cur])){ // @warning: obviously, the connection is just a rough estimation
                                nodes[v].fpga = fpgaVec[cur];
                                fpgas[fpgaVec[cur]].nodes.push_back(v);
                                q.push(v);
                            }
                        }
                    }
                }
                cur = (cur + 1) % K;
                it = nodeVec.erase(it);
                break;
            }
        }
    }
    if (!nodeVec.empty()){
        for(auto node : nodeVec){
            nodes[node].fpga = dis(global_rng);
            fpgas[nodes[node].fpga].nodes.push_back(node);
            _fpga_add_res_force(nodes[node].fpga, node);
            _fpga_cal_conn_force(nodes[node].fpga);
        }
    }
}

void ParFunc::initial_partition(){
    bfs_partition();
}

// void ParFunc::map_fpga(){
//     // @todo: mapping fpga to the coarsest partition
//     // maybe greedy algorithm is enough
//     // currently, we just randomly assign nodes to fpgas
// }