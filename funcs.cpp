#include <funcs.hpp>
#include <cassert>
#include <cmath>
#include <cstring>
#include <algorithm>
#include <iterator>
#include <numeric>
#include <random>
#include <queue>

std::random_device rd;
std::mt19937 global_rng(rd());

void ParFunc::_contract(LoReHyPar &hp, int u, int v){
    contMeme.push({u, v});
    curNodes.erase(v);
    delNodes.insert(v);
    auto &nodes = hp.nodes;
    auto &nets = hp.nets;
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] += nodes[v].resLoad[i];
    }
    for (int net : nodes[v].nets){
        if (nets[net].size == 1){ // v is the only node in the net, relink (u, net)
            nets[net].nodes[0] = u;
            nets[net].source = u;
            nodes[u].nets.insert(net);
            nodes[u].isSou[net] = true;
            netFp[net] += u * u - v * v; // incrementally update the fingerprint
        }else{
            auto vit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), v);
            if (nodes[v].isSou[net]){
                nets[net].source = u;
                nodes[u].isSou[net] = true;
            }
            if (!nodes[u].nets.count(net)){ // u is not in the net, relink (u, net)
                *vit = u;
                nodes[u].nets.insert(net);
                netFp[net] += u * u - v * v; // incrementally update the fingerprint
            }
            else{ // u is in the net, delete v
                *vit = nets[net].nodes[--nets[net].size];
                nets[net].nodes[nets[net].size] = v;
                netFp[net] -= v * v; // incrementally update the fingerprint
            }
        }
    }
}

void ParFunc::_uncontract(LoReHyPar &hp, int u, int v){
    assert(contMeme.top() == std::make_pair(u, v));
    contMeme.pop();
    curNodes.insert(v);
    delNodes.erase(v);
    auto &nodes = hp.nodes;
    auto &nets = hp.nets;
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] -= nodes[v].resLoad[i];
    }
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    for (int net : secNets){
        if (nodes[v].isSou[net]){
            nets[net].source = v;
            nodes[u].isSou[net] = false;
        }
        if (nets[net].nodes.size() == nets[net].size || nets[net].nodes[nets[net].size] != v){ // v is relinked
            auto uit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u);
            *uit = v;
            nodes[u].nets.erase(net);
        }else{ // v is deleted
            ++nets[net].size;
        }
    }
}

// @warning: the following functions haven't been tested by Haichuan liu!
// @todo: check the correctness of the following functions

inline float ParFunc::_heavy_edge_rating(LoReHyPar &hp, int u, int v){
    auto &nodes = hp.nodes;
    auto &nets = hp.nets;
    std::set<int> secNets;
    std::set_intersection(nodes[u].nets.begin(), nodes[u].nets.end(), nodes[v].nets.begin(), nodes[v].nets.end(), std::inserter(secNets, secNets.begin()));
    float rating = 0;
    for (int net : secNets){
        if (nets[net].size > static_cast<size_t>(parameter_l)){
            continue;
        }
        rating += (float) nets[net].weight / (nets[net].size - 1); // nets[net].size >= 2
    }
    return rating;
}

void ParFunc::_init_net_fp(LoReHyPar &hp){
    auto &nets = hp.nets;
    for (size_t i = 0; i < nets.size(); ++i){
        netFp[i] = 0;
        for (int node : nets[i].nodes){
            netFp[i] += node * node;
        }
    }
}

void ParFunc::_detect_para_singv_net(LoReHyPar &hp){
    auto &nets = hp.nets;
    // for a deleted net, do we need to record it and plan to recover it?
    // will deleting a net fail the uncontract operation?
    // maybe we need to record the order of deletion and the level
    for (size_t i = 0; i < nets.size(); ++i){
        if (nets[i].size == 1){
            netFp[i] = 0; // delete single vertex net
        }
    }
    std::vector<int> indices(nets.size());
    std::iota(indices.begin(), indices.end(), 0); 
    std::sort(indices.begin(), indices.end(), [&](int i, int j){return netFp[i] < netFp[j];});
    bool paraFlag = false;
    int paraNet = -1, fp = -1;
    for (int i : indices){
        if (netFp[i] == 0){
            continue;
        }
        if (!paraFlag){
            fp = netFp[i];
            paraNet = i;
        } else if (netFp[i] == fp){
            paraFlag = true;
            nets[paraNet].weight += nets[i].weight; // merge parallel nets
            netFp[i] = 0;
        } else{
            paraFlag = false;
            fp = -1;
            paraNet = -1;
        }
    }
    // finally, do we really need this function?
}

void ParFunc::_init_ceil_mean_res(LoReHyPar &hp){
    memset(ceilRes, 0, sizeof(ceilRes));
    memset(meanRes, 0, sizeof(meanRes));
    auto &fpgas = hp.fpgas;
    for (int i = 0; i < NUM_RES; ++i){
        for (auto &fpga : fpgas){
            ceilRes[i] = std::max(ceilRes[i], static_cast<int>(std::ceil((float) fpga.resCap[i] / k * parameter_t)));
            meanRes[i] += fpga.resCap[i];
        }
        meanRes[i] = static_cast<int>(std::ceil(meanRes[i] / k ));
    }
}

bool ParFunc::_contract_eligible(LoReHyPar &hp, int u, int v){
    auto &nodes = hp.nodes;
    for (int i = 0; i < NUM_RES; ++i){
        if (nodes[u].resLoad[i] + nodes[v].resLoad[i] > ceilRes[i]){
            return false;
        }
    }
    return true;
}

bool ParFunc::_fpga_add_res(LoReHyPar &hp, int f, int u){
    auto &fpgas = hp.fpgas;
    auto &nodes = hp.nodes;
    int tmp[NUM_RES];
    std::copy(fpgas[f].resUsed, fpgas[f].resUsed + NUM_RES, tmp);
    for (int i = 0; i < NUM_RES; ++i){
        fpgas[f].resUsed[i] += nodes[u].resLoad[i];
        if (fpgas[f].resUsed[i] > fpgas[f].resCap[i]){
            std::copy(tmp, tmp + NUM_RES, fpgas[f].resUsed);
            return false;
        }
    }
    return true;
}

// @todo: now full update, maybe in the future we can incrementally update the connection
bool ParFunc::_fpga_cal_conn(LoReHyPar &hp, int f){
    auto &fpgas = hp.fpgas;
    auto &nodes = hp.nodes;
    auto &nets = hp.nets;
    std::unordered_map<int, bool> netCaled;
    int conn = 0;
    for (int node : fpgas[f].nodes){
        for (int net : nodes[node].nets){
            if (netCaled.count(net)){
                continue;
            }
            netCaled[net] = true;
            for (int v : nets[net].nodes){
                if (nodes[v].fpga != f){
                    conn += nets[net].weight;
                    if (conn > fpgas[f].maxConn){
                        return false;
                    }
                    break;
                }
            }
        }
    }
    fpgas[f].usedConn = conn;
    return true;
}

void ParFunc::_test_contract(LoReHyPar &hp){
    int u = 0;
    auto &nodes = hp.nodes;
    for (size_t v = 1; v < nodes.size(); ++v){
        _contract(hp, u, v);
        std::cout << "Contract " << u << " " << v << std::endl;
        hp.printSummary(std::cout);
    }
    while (!contMeme.empty()){
        auto [u, v] = contMeme.top();
        _uncontract(hp, u, v);
        std::cout << "Uncontract " << u << " " << v << std::endl;
        hp.printSummary(std::cout);
    }
}

void ParFunc::preprocessing(LoReHyPar &hp){
    // @todo: preprocessing function
}

bool ParFunc::coarsen_naive(LoReHyPar &hp){
    bool contFlag = false;
    std::vector<int> randNodes(curNodes.begin(), curNodes.end());
    std::shuffle(randNodes.begin(), randNodes.end(), global_rng);
    for (int u : randNodes){
        if (delNodes.count(u)){
            continue;
        }
        float maxRating = 0;
        int v = -1;
        for (int w : curNodes){
            if (w == u || !_contract_eligible(hp, u, w)){
                continue;
            }
            float rating = _heavy_edge_rating(hp, u, w);
            if (rating > maxRating){
                maxRating = rating;
                v = w;
            }
        }
        if (v != -1){
            _contract(hp, u, v);
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                                                                            
}

void ParFunc::coarsen(LoReHyPar &hp){
    _init_net_fp(hp);
    _init_ceil_mean_res(hp);
    while (curNodes.size() >= static_cast<size_t>(k * parameter_t)){
        if (!coarsen_naive(hp)){
            break;
        }
    }
}

void ParFunc::bfs_partition(LoReHyPar &hp){
    std::vector<int> nodeVec(curNodes.begin(), curNodes.end());
    std::shuffle(nodeVec.begin(), nodeVec.end(), global_rng);
    std::vector<int> fpgaVec(k);
    std::iota(fpgaVec.begin(), fpgaVec.end(), 0);
    std::shuffle(fpgaVec.begin(), fpgaVec.end(), global_rng);
    auto &nodes = hp.nodes;
    auto &nets = hp.nets;
    auto &fpgas = hp.fpgas;
    int cur = 0;
    for (int node : nodeVec){
        if (nodes[node].fpga != -1){
            continue;
        }
        int f = fpgaVec[cur++];
        nodes[node].fpga = f;
        fpgas[f].nodes.push_back(node); // @todo: when the fpga can't hold the only node, how to handle this?
        _fpga_add_res(hp, f, node);
        std::queue<int> q;
        while(!q.empty()){
            int u = q.front();
            q.pop();
            for (int net : nodes[u].nets){
                for (int v : nets[net].nodes){
                    if (nodes[v].fpga == -1 && _fpga_add_res(hp, f, v) && _fpga_cal_conn(hp, f)){ // @warning: obviously, the connection is just a rough estimation
                        nodes[v].fpga = f;
                        fpgas[f].nodes.push_back(v);
                        q.push(v);
                    }
                }
            }
        }
    }
}

void ParFunc::initial_partition(LoReHyPar &hp){
    bfs_partition(hp);
}

void ParFunc::map_fpga(LoReHyPar &hp){
    // @todo: mapping fpga to the coarsest partition
    // maybe greedy algorithm is enough
    // currently, we just randomly assign nodes to fpgas
} 

void ParFunc::refine(LoReHyPar &hp){
    // @warning: the following code is just uncontract all nodes
    auto &fpgas = hp.fpgas;
    auto &nodes = hp.nodes;
    while (!contMeme.empty()){
        auto [u, v] = contMeme.top();
        _uncontract(hp, u, v);
        int f = hp.nodes[u].fpga;
        nodes[v].fpga = f;
        fpgas[f].nodes.push_back(v);
    }
}