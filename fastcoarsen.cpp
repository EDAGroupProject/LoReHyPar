#include <hypar.hpp>
#include <algorithm>


bool HyPar::fast_coarsen_in_community(int community) {
    bool contFlag = false;
    std::priority_queue<std::tuple<int, int, int>> rating_map;
    std::unordered_map<std::pair<int, int>, int, pair_hash> rating;
    for (int u : communities[community]) {
        for (int v : communities[community]) {
            if (u >= v) {
                continue;
            } else if(_heavy_edge_rating(u, v, rating)) {
                rating_map.push({rating[{u, v}], u, v});
            }
        }
    }
    for (auto [r, u, v] = rating_map.top(); !rating_map.empty(); rating_map.pop(), std::tie(r, u, v) = rating_map.top()) {
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

void HyPar::fast_coarsen() {
    std::vector<int> communiy_vec(communities.size());
    std::iota(communiy_vec.begin(), communiy_vec.end(), 0);
    std::sort(communiy_vec.begin(), communiy_vec.end(), [&](int i, int j) {return communities[i].size() > communities[j].size();});
    for (int c : communiy_vec) {
        if (existing_nodes.size() < static_cast<size_t>(K * parameter_t)) {
            break;
        }
        while (naive_coarsen_in_community(c) && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
            continue;
        }
    }
    std::cout << "After coarsen: " << existing_nodes.size() << std::endl;
}