#include <hypar.hpp>
#include <algorithm>
#include <iterator>

void LoReHyPar::_contract(int u, int v){
    cont_meme.push({u, v});
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] += nodes[v].resLoad[i];
    }
    for (int net: nodes[v].nets){
        if (nets[net].size == 1){ // v is the only node in the net, relink (u, net)
            nets[net].nodes[0] = u;
            nets[net].source = u;
            nodes[u].nets.emplace_back(net);
            nodes[u].isSou[net] = true;
        }else {
            size_t id1 = std::distance(nets[net].nodes.begin(), std::find(nets[net].nodes.begin(), nets[net].nodes.end(), v));
            size_t id2 = std::distance(nets[net].nodes.begin(), std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u));
            if (nodes[v].isSou[net]){
                nets[net].source = u;
                nodes[u].isSou[net] = true;
            }
            if (id2 >= nets[net].size){ // u is not in the net, relink (u, net)
                nets[net].nodes[id1] = u;
                nodes[u].nets.emplace_back(net);
            }else { // u is in the net, delete v
                nets[net].nodes[id1] = nets[net].nodes[--nets[net].size];
                nets[net].nodes[nets[net].size] = v;
            }
        }
    }
}

void LoReHyPar::_uncontract(int u, int v){
    cont_meme.pop();
    for (int i = 0; i < NUM_RES; ++i){
        nodes[u].resLoad[i] -= nodes[v].resLoad[i];
    }
    std::unordered_map<int, bool> vNet{};
    for (int net: nodes[v].nets){
        vNet[net] = true;
    }
    for (auto eit = nodes[u].nets.begin(); eit != nodes[u].nets.end();){
        int net = *eit;
        if (vNet[net]){
            if (nodes[v].isSou[net]){
                nets[net].source = v;
                nodes[u].isSou[net] = false;
            }
            if (nets[net].nodes.size() == nets[net].size || nets[net].nodes[nets[net].size] != v){ // v is relinked
                auto uit = std::find(nets[net].nodes.begin(), nets[net].nodes.end(), u);
                *uit = v;
                eit = nodes[u].nets.erase(eit);
            }else { // v is deleted
                ++nets[net].size;
                ++eit;
            }
        }else {
            ++eit;
        }
    }
}