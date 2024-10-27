#include <hypar.hpp>
#include <algorithm>

bool HyPar::_coarsen_naive(){
    bool contFlag = false;
    std::vector<int> randNodes(existing_nodes.begin(), existing_nodes.end());
    std::shuffle(randNodes.begin(), randNodes.end(), get_rng());
    for (int u : randNodes){
        if (deleted_nodes.count(u)){
            continue;
        }
        float maxRating = 0.0;
        int v = -1;
        for (int net : nodes[u].nets){
            for (int i = 0; i < nets[net].size; ++i){
                int w = nets[net].nodes[i];
                if (w == u || deleted_nodes.count(w) || !_contract_eligible(u, w)){
                    continue;
                }
                float rating = _heavy_edge_rating(u, w);
                if (rating > maxRating){
                    maxRating = rating;
                    v = w;
                }
            }
        }
        if (v != -1){
            _contract(u, v);
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                                                                            
}

void HyPar::coarsen_naive(){
    while (existing_nodes.size() >= static_cast<size_t>(K * parameter_t)){
        if (!_coarsen_naive()){
            break;
        }
    }
}

bool HyPar::coarsen_in_community(int community){
    bool contFlag = false;
    std::vector<int> randNodes(commuinities[community].begin(), commuinities[community].end());
    std::shuffle(randNodes.begin(), randNodes.end(), get_rng());
    for (int u : randNodes){
        if (deleted_nodes.count(u)){
            continue;
        }
        float maxRating = 0;
        int v = -1;
        for (int net : nodes[u].nets){
            for (int i = 0; i < nets[net].size; ++i){
                int w = nets[net].nodes[i];
                if (w == u || node2community[w] != node2community[u] || deleted_nodes.count(w) || !_contract_eligible(u, w)){
                    continue;
                }
                float rating = _heavy_edge_rating(u, w);
                if (rating > maxRating){
                    maxRating = rating;
                    v = w;
                }
            }
        }
        if (v != -1){
            _contract(u, v);
            commuinities[community].erase(v); // additionally, we need to update the community
            contFlag = true;
        }
    }
    return contFlag; 
}

void HyPar::coarsen(){
    while (existing_nodes.size() >= static_cast<size_t>(K * parameter_t)){
        bool contFlag = false;
        for (size_t i = 0; i < commuinities.size(); ++i){
            if (coarsen_in_community(i)){
                contFlag = true;
            }
        }
        if (!contFlag){
            break;
        }
    }
}