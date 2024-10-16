#include <hypar.hpp>
#include <algorithm>

bool ParFunc::coarsen_naive(){
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
            if (w == u || !_contract_eligible(u, w)){
                continue;
            }
            float rating = _heavy_edge_rating(u, w);
            if (rating > maxRating){
                maxRating = rating;
                v = w;
            }
        }
        if (v != -1){
            _contract(u, v);
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                                                                            
}

void ParFunc::coarsen(){
    _init_net_fp();
    _init_ceil_mean_res();
    curNodes.clear();
    delNodes.clear();
    edgeRatng.clear();
    for(size_t i = 0; i < nodes.size(); ++i){
        curNodes.insert(i);
    }
    while (curNodes.size() >= static_cast<size_t>(K * parameter_t)){
        if (!coarsen_naive()){
            break;
        }
    }
}