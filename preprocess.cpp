#include <hypar.hpp>
#include <LSHConstants.hpp>
#include <Louvain.hpp>
#include <cmath>
#include <algorithm>

void HyPar::pin_sparsify() {
    int hash_num = NUM_HASHES;
    int c_min =static_cast<int>(std::sqrt(parameter_l)), c_max = static_cast<int>(std::ceil(double(nodes.size()) / (K * parameter_t))); // @warning: the range of c is arbitrary now
    std::unordered_set<int> active_nodes(existing_nodes);
    int hash_min = 3, hash_max = hash_min + double(K) /( 1 + std::log(nodes.size()));
    std::mt19937 rng = get_rng();
    std::uniform_int_distribution<int> dis(0, hash_num - 1);
    for (int hash_sel = hash_min; hash_sel <= hash_max; ++hash_sel) {
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
        std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
        for (int node : nodeVec) { // Merge neighbors with the same hash
            if (!active_nodes.count(node)) {
                continue;
            }
            std::unordered_map<int, bool> vis;
            for (int net : nodes[node].nets) {
                for (int j = 0; j < nets[net].size; ++j) {
                    int neighbor = nets[net].nodes[j];
                    if (!active_nodes.count(neighbor) || vis[neighbor] || neighbor == node) {
                        continue;
                    }
                    vis[neighbor] = true;
                    if (hashTable[node] == hashTable[neighbor]) {
                        _contract_with_nodefp(node, neighbor);
                        active_nodes.erase(neighbor);
                    } 
                }
                if (nodes[node].size >= c_max) {
                    break;
                }
            }
            if (nodes[node].size >= c_min) {
                active_nodes.erase(node);
            }
        }
    }
}

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
                    _contract_with_nodefp(u, nodeVec[i]);
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

void HyPar::fast_pin_sparsify_in_community(int community, int c_min) {
    int hash_num = NUM_HASHES;
    std::unordered_set<int> active_nodes(communities[community]);
    std::mt19937 rng = get_rng();
    std::uniform_int_distribution<int> dis(0, hash_num - 1);
    for (int hash_sel = 4; hash_sel >= 1; --hash_sel) {
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
                if (_contract_eligible(u, nodeVec[i])) {
                    _contract_with_nodefp(u, nodeVec[i]);
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

void HyPar::fast_pin_sparsify_in_community(std::unordered_set<int> &community, int c_min, int c_max) {
    int hash_num = NUM_HASHES;
    std::unordered_set<int> active_nodes(community);
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
                if (nodes[u].size + nodes[nodeVec[i]].size <= c_max && _contract_eligible(u, nodeVec[i])) {
                    _contract_with_nodefp(u, nodeVec[i]);
                    active_nodes.erase(nodeVec[i]);
                    community.erase(nodeVec[i]);
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

void HyPar::pin_sparsify_in_community(int community, int c_min, int c_max) {
    int hash_num = NUM_HASHES; 
    std::unordered_set<int> active_nodes(communities[community]);
    std::mt19937 rng = get_rng();
    std::uniform_int_distribution<int> dis(0, hash_num - 1);
    for (int hash_sel = 3; hash_sel >= 1; --hash_sel) {
        size_t old_size = active_nodes.size();
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
        std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
        for (int node : nodeVec) {
            if (!active_nodes.count(node)) {
                continue;
            }
            std::unordered_map<int, bool> vis;
            for (int net : nodes[node].nets) {
                for (int j = 0; j < nets[net].size; ++j) {
                    int neighbor = nets[net].nodes[j];
                    if (!active_nodes.count(neighbor) || vis[neighbor] || neighbor == node) {
                        continue;
                    }
                    vis[neighbor] = true;
                    if (hashTable[node] == hashTable[neighbor]) {
                        _contract_with_nodefp(node, neighbor);
                        communities[community].erase(neighbor);
                        active_nodes.erase(neighbor);
                    }
                }
                if (nodes[node].size >= c_max) {
                    break;
                }
            }
            if (nodes[node].size >= c_min) {
                active_nodes.erase(node);
            }
        }
        if (active_nodes.size() == old_size) {
            break;
        }
    }
}

int HyPar::community_detect() {
    Louvain lv(nets.size() + existing_nodes.size());
    std::vector<int> louvain2node;
    double edge_density = double(nets.size()) / nodes.size();
    if (edge_density >= 0.75) {
        for (int node : existing_nodes) {
            louvain2node.push_back(node);
            for (int net : nodes[node].nets) {
                lv.addEdge(nets.size() + node, net, 1.0);
            }
        }
    } else {
        for (int node : existing_nodes) {
            louvain2node.push_back(node);
            size_t d = nodes[node].nets.size();
            for (int net : nodes[node].nets) {
                lv.addEdge(nets.size() + node, net, double(d) / nets[net].size);
            }
        }
    }
    lv.run();
    int max_size = 0;
    for (int c : lv.nodes) {
        bool flag = false;
        for (int u : lv.communityNodes[c]) {
            if (u >= static_cast<int>(nets.size())) {
                if (!flag) {
                    flag = true;
                    communities.push_back({});
                }
                int node = louvain2node[u - nets.size()];
                communities.back().insert(node);
            }
        }
        if (flag) {
            max_size = std::max(max_size, static_cast<int>(communities.back().size()));
        }
    }
    return max_size;
}

void HyPar::contract_in_community(int community) {
    auto it = communities[community].begin();
    int u = *it;
    ++it;
    for (; it != communities[community].end(); ) {
        int v = *it;
        _contract_with_nodefp(u, v);
        it = communities[community].erase(it);
    }
} 

void HyPar::contract_in_community(const std::unordered_set<int> &community, std::unordered_set<int> &active_nodes) {
    auto it = community.begin();
    int u = *it;
    ++it;
    for (; it != community.end(); ++it) {
        int v = *it;
        _contract_with_nodefp(u, v);
        active_nodes.erase(v);
    }
}

void HyPar::preprocess() {
    for(size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
    community_detect();
    for (size_t i = 0; i < communities.size(); ++i) {
        for (int u : communities[i]) {
            node2community[u] = i;
        }
    }
}

void HyPar::fast_preprocess() {
    for(size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
    int max_size = community_detect();
    int c_min = static_cast<int>(std::ceil(double(max_size) / (10 * parameter_t)));
    for (size_t i = 0; i < communities.size(); ++i) {
        if (static_cast<int>(communities[i].size()) > c_min) {
            fast_pin_sparsify_in_community(i, c_min);
        }
        for (int u : communities[i]) {
            node2community[u] = i;
        }
    }
    std::cout << "After community contract: " << existing_nodes.size() << std::endl;
}