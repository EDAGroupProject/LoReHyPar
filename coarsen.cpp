#include <hypar.hpp>

bool HyPar::coarsen_by_nets() {
    bool contFlag = false;
    std::priority_queue<std::tuple<int, int, int>> rating_map;
    std::unordered_map<std::pair<int, int>, int, pair_hash> rating;
    for (const auto &net : nets) {
        if (net.size > parameter_l || net.size == 1) {
            continue;
        }
        for (int i = 0; i < net.size; ++i) {
            int u = net.nodes[i];
            for (int j = i + 1; j < net.size; ++j) {
                int v = net.nodes[j];
                rating[{u, v}] += net.weight / (net.size - 1);
            }
        }
    }
    for (const auto& [key, r] : rating) {
        rating_map.push({r, key.first, key.second});
    }
    std::unordered_map<int, bool> deleted;
    while (!rating_map.empty() && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
        auto [r, u, v] = rating_map.top();
        rating_map.pop();
        if (deleted[u] || deleted[v]) {
            continue;
        }
        if (_contract_eligible(u, v)) {
            _contract(u, v);
            deleted[v] = true;
            contFlag = true;
        }
    }
    return contFlag;                                                                                                                                                                                     
}

void HyPar::coarsen() {
    while (coarsen_by_nets() && existing_nodes.size() >= static_cast<size_t>(K * parameter_t)) {
        continue;
    }
}
