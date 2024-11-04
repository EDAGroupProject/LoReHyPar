#include <hypar.hpp>
#include <LSHConstants.hpp>
#include <Louvain.hpp>
#include <cmath>
#include <algorithm>

bool comp(const std::vector<int> &u, const std::vector<int> &v) {
    if (u.size() != v.size()) {
        return u.size() < v.size();
    } else {
        for (size_t i = 0; i < u.size(); ++i) {
            if (u[i] != v[i]) {
                return u[i] < v[i];
            }
        }
    }
    return false;
}


void HyPar::fast_pin_sparsify() {
    int hash_num = NUM_HASHES;
    int c_min = static_cast<int>(std::sqrt(parameter_l));
    int c_max = static_cast<int>(std::ceil(double(nodes.size()) / (K * K * parameter_tau)));
    std::unordered_set<int> active_nodes(existing_nodes);
    std::mt19937 rng = get_rng();
    std::uniform_int_distribution<int> dis(0, hash_num - 1);
    for (int hash_sel = 3; hash_sel >= 1; --hash_sel) {
        std::cout << existing_nodes.size() << " " << active_nodes.size() << std::endl;
        std::vector<int> slice(hash_sel);
        for (int i = 0; i < hash_sel; ++i) {
            int rd = dis(rng);
            while (std::find(slice.begin(), slice.end(), rd) != slice.end()) {
                rd = dis(rng);
            }
            slice[i] = rd;
        }
        std::unordered_map<int, std::vector<int>> hashTable;
        for (int node : active_nodes) {
            for (int i = 0; i < hash_sel; ++i) {
                int hash_id = slice[i];
                hashTable[node].emplace_back((A_VALUES[hash_id] * nodes[node].fp + B_VALUES[hash_id]) % P_VALUES[hash_id]);
            }
        }
        std::vector<int> nodeVec(active_nodes.begin(), active_nodes.end());
        std::sort(nodeVec.begin(), nodeVec.end(), [&](int u, int v) {
            return comp(hashTable[u], hashTable[v]);
        });
        auto hash = hashTable[nodeVec[0]];
        int u = nodeVec[0];
        for (size_t i = 1; i < nodeVec.size(); ++i) {
            if (hashTable[nodeVec[i]] == hash) {
                if (nodes[u].size + nodes[nodeVec[i]].size <= c_max) {
                    _contract(u, nodeVec[i]);
                    active_nodes.erase(nodeVec[i]);
                } else {
                    active_nodes.erase(u);
                    u = nodeVec[i];
                }
            } else {
                if (nodes[u].size >= c_min) {
                    active_nodes.erase(u);
                }
                hash = hashTable[nodeVec[i]];
                u = nodeVec[i];
            }
        }
    }
}

void HyPar::fast_pin_sparsify_in_community(int community, int c_min, int c_max) {
    int hash_num = NUM_HASHES;
    std::unordered_set<int> active_nodes(communities[community]);
    std::mt19937 rng = get_rng();
    std::uniform_int_distribution<int> dis(0, hash_num - 1);
    for (int hash_sel = 4; hash_sel >= 2; --hash_sel) {
        std::vector<int> slice(hash_sel);
        for (int i = 0; i < hash_sel; ++i) {
            int rd = dis(rng);
            while (std::find(slice.begin(), slice.end(), rd) != slice.end()) {
                rd = dis(rng);
            }
            slice[i] = rd;
        }
        std::unordered_map<int, std::vector<int>> hashTable;
        for (int node : active_nodes) {
            for (int i = 0; i < hash_sel; ++i) {
                int hash_id = slice[i];
                hashTable[node].emplace_back((A_VALUES[hash_id] * nodes[node].fp + B_VALUES[hash_id]) % P_VALUES[hash_id]);
            }
        }
        std::vector<int> nodeVec(active_nodes.begin(), active_nodes.end());
        std::sort(nodeVec.begin(), nodeVec.end(), [&](int u, int v) {
            return comp(hashTable[u], hashTable[v]);
        });
        auto hash = hashTable[nodeVec[0]];
        int u = nodeVec[0];
        for (size_t i = 1; i < nodeVec.size(); ++i) {
            if (hashTable[nodeVec[i]] == hash) {
                if (nodes[u].size + nodes[nodeVec[i]].size <= c_max) {
                    _contract(u, nodeVec[i]);
                    active_nodes.erase(nodeVec[i]);
                    communities[community].erase(nodeVec[i]);
                } else {
                    active_nodes.erase(u);
                    u = nodeVec[i];
                }
            } else {
                if (nodes[u].size >= c_min) {
                    active_nodes.erase(u);
                }
                hash = hashTable[nodeVec[i]];
                u = nodeVec[i];
            }
        }
    }
}

void HyPar::preprocess_for_next_round() {
    existing_nodes.clear();
    deleted_nodes.clear();
    for(size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
    int max_size = community_detect();
    int c_min = static_cast<int>(std::ceil(double(max_size) / (10)));
    int c_max = static_cast<int>(std::ceil(double(nodes.size()) / (K * parameter_t)));
    std::unordered_set<int> another_community;
    for (size_t i = 0; i < communities.size(); ++i) {
        if (static_cast<int>(communities[i].size()) <= c_min) {
            another_community.insert(communities[i].begin(), communities[i].end());
            communities[i].clear();
        } else {
            fast_pin_sparsify_in_community(i, c_min, c_max);
        }
    }
    for (auto it = communities.begin(); it != communities.end();) {
        if (it->empty()) {
            it = communities.erase(it);
        } else {
            ++it;
        }
    }
    communities.emplace_back(another_community);
    fast_pin_sparsify_in_community(communities.size() - 1, c_min, c_max);
}