#include <hypar.hpp>

void ParFunc::refine(){
    // @warning: the following code is just uncontract all nodes
    while (!contMeme.empty()){
        auto [u, v] = contMeme.top();
        _uncontract(u, v);
        int f = nodes[u].fpga;
        nodes[v].fpga = f;
        fpgas[f].nodes.push_back(v);
    }
}