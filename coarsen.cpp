#include <hypar.hpp>
#include <algorithm>

bool HyPar::_coarsen_naive() {
    bool contFlag = false;
    std::vector<int> randNodes(existing_nodes.begin(), existing_nodes.end());
    std::shuffle(randNodes.begin(), randNodes.end(), get_rng());
    for (int u : randNodes) {
        if (deleted_nodes.count(u)) {
            continue;
        }
        float maxRating = 0.0;
        int v = -1;
        for (int net : nodes[u].nets) {
            for (int i = 0; i < nets[net].size; ++i) {
                int w = nets[net].nodes[i];
                if (w == u || deleted_nodes.count(w) || !_contract_eligible(u, w)) {
                    continue;
                }
                float rating = _heavy_edge_rating(u, w);
                if (rating > maxRating) {
                    maxRating = rating;
                    v = w;
                }
            }
        }
        if (v != -1) {
            _contract(u, v);
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                                                                            
}

void HyPar::coarsen_naive() {
    _init_ceil_res();
    while (existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
        if (!_coarsen_naive()) {
            break;
        }
    }
}

bool HyPar::coarsen_in_community(int community) {
    bool contFlag = false;
    std::vector<int> randNodes(communities[community].begin(), communities[community].end());
    std::shuffle(randNodes.begin(), randNodes.end(), get_rng());
    for (int u : randNodes) {
        if (deleted_nodes.count(u)) {
            continue;
        }
        float maxRating = 0.0;
        int v = -1;
        std::unordered_map<int, bool> vis;
        for (int net : nodes[u].nets) {
            for (int i = 0; i < nets[net].size; ++i) {
                int w = nets[net].nodes[i];
                if (w == u || !communities[community].count(w) || vis[w] || !_contract_eligible(u, w)) {
                    continue;
                }
                vis[w] = true;
                float rating = _heavy_edge_rating(u, w);
                if (rating > maxRating) {
                    maxRating = rating;
                    v = w;
                }
            }
        }
        if (v != -1) {
            _contract(u, v);
            communities[community].erase(v); // additionally, we need to update the community
            contFlag = true;
        }
    }
    return contFlag; 
}

bool HyPar::fast_coarsen_in_community(int community) {
    bool contFlag = false;
    std::vector<std::pair<int, int>> node_pairs;
    std::unordered_map<std::pair<int, int>, int, pair_hash> rating;
    for (int u : communities[community]) {
        for (int v : communities[community]) {
            if (u >= v) {
                continue;
            } else if(_heavy_edge_rating(u, v, rating)) {
                node_pairs.emplace_back(u, v);
            }
        }
    }
    std::sort(node_pairs.begin(), node_pairs.end(), [&](const std::pair<int, int> &a, const std::pair<int, int> &b) {
        return rating[a] > rating[b];
    });
    for (auto [u, v] : node_pairs) {
        if (deleted_nodes.count(u) || deleted_nodes.count(v)) {
            continue;
        }
        if (_contract_eligible(u, v)) {
            _contract(u, v);
            communities[community].erase(v);
            contFlag = true;
        }
    }
    return contFlag; 
}

bool HyPar::naive_coarsen_in_community(int community) {
    bool conFlag = false;
    std::vector<int> randNodes(communities[community].begin(), communities[community].end());
    std::shuffle(randNodes.begin(), randNodes.end(), get_rng());
    for (int u : randNodes) {
        if (!communities[community].count(u)) {
            continue;
        }
        for (int net : nodes[u].nets) {
            for (int i = 0; i < nets[net].size; ++i) {
                int w = nets[net].nodes[i];
                if (w == u || !communities[community].count(w) || !_contract_eligible(u, w)) {
                    continue;
                }
                conFlag = true;
                _contract(u, w);
                communities[community].erase(w);
            }
        }
    }
    return conFlag;
}

void HyPar::coarsen() {
    _init_ceil_res();
    std::vector<int> communiy_vec(communities.size());
    std::iota(communiy_vec.begin(), communiy_vec.end(), 0);
    std::sort(communiy_vec.begin(), communiy_vec.end(), [&](int i, int j) {return communities[i].size() > communities[j].size();});
    // for (int c : communiy_vec) {
    //     if (existing_nodes.size() < static_cast<size_t>(K * parameter_t)) {
    //         break;
    //     }
    //     coarsen_in_community_arbitary(c, 0.5);
    // }
    for (int c : communiy_vec) {
        if (existing_nodes.size() < static_cast<size_t>(K * parameter_t)) {
            break;
        }
        while (fast_coarsen_in_community(c) && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
            continue;
        }
    }
    // for (int c : communiy_vec) {
    //     if (existing_nodes.size() < static_cast<size_t>(K * parameter_t)) {
    //         break;
    //     }
    //     while (naive_coarsen_in_community(c) && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
    //         continue;
    //     }
    // }
}