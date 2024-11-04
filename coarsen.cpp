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

bool HyPar::coarsen_by_nets() {
    bool contFlag = false;
    std::priority_queue<std::tuple<int, int, int>> rating_map;
    std::unordered_map<std::pair<int, int>, int, pair_hash> rating;
    for (auto &net : nets) {
        if (net.size > parameter_l || net.size == 1) {
            continue;
        }
        for (int i = 0; i < net.size; ++i) {
            int u = net.nodes[i];
            for (int j = i + 1; j < net.size; ++j) {
                if (node2community[u] != node2community[net.nodes[j]]) {
                    continue;
                }
                int v = net.nodes[j];
                rating[{u, v}] += net.weight / (net.size - 1);
            }
        }
    }
    for (const auto& [key, r] : rating) {
        rating_map.push({r, key.first, key.second});
    }
    while (!rating_map.empty() && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
        auto [r, u, v] = rating_map.top();
        rating_map.pop();
        if (deleted_nodes.count(u) || deleted_nodes.count(v)) {
            continue;
        }
        if (_contract_eligible(u, v)) {
            _contract(u, v);
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                                                                            
}

void HyPar::coarsen() {
    while (coarsen_by_nets() && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
        continue;
    }
    std::cout << "After coarsen: " << existing_nodes.size() << std::endl;
}

