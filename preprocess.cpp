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
                        _contract(node, neighbor);
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
                        _contract(node, neighbor);
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
    float edge_density = float(nets.size()) / nodes.size();
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
                lv.addEdge(nets.size() + node, net, float(d) / nets[net].size);
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
        _contract(u, v);
        it = communities[community].erase(it);
    }
} 

void HyPar::contract_in_community(const std::unordered_set<int> &community, std::unordered_set<int> &active_nodes) {
    auto it = community.begin();
    int u = *it;
    ++it;
    for (; it != community.end(); ++it) {
        int v = *it;
        _contract(u, v);
        active_nodes.erase(v);
    }
}

void HyPar::recursive_community_contract() {
    std::unordered_set<int> active_nodes(existing_nodes);
    std::vector<int> community_sizes;
    bool end_flag = false;
    size_t ceil = static_cast<size_t>(std::ceil(double(nodes.size()) / (10)));
    size_t old_size = -1;
    while (!end_flag || active_nodes.size() > ceil || existing_nodes.size() != old_size) {
        std::cout<< active_nodes.size() << " " << communities.size() <<std::endl;
        end_flag = true;
        old_size = existing_nodes.size();
        Louvain lv(nets.size() + active_nodes.size());
        std::vector<int> louvain2node;
        for (int node : active_nodes) {
            louvain2node.push_back(node); 
            for (int net : nodes[node].nets) {
                lv.addEdge(nets.size() + node, net, 1.0);
            }
        }
        lv.run();
        communities.clear();
        community_sizes.clear();
        for (int c : lv.nodes) {
            std::unordered_set<int> community;
            int size = 0;
            for (int u : lv.communityNodes[c]) {
                if (u >= static_cast<int>(nets.size())) {
                    int node = louvain2node[u - nets.size()];
                    community.insert(node);
                    size += nodes[node].size;
                }
            }
            if (community.empty()) {
                continue;
            }
            community_sizes.push_back(size);
            communities.push_back(community);
        }
        int c_min = static_cast<int>(std::ceil(double(nodes.size()) / (K * parameter_t * communities.size())));
        int c_max = static_cast<int>(std::ceil(double(nodes.size()) / (communities.size())));
        for (size_t i = 0; i < communities.size(); ++i) {
            if (community_sizes[i] <= c_max) {
                end_flag = false;
                contract_in_community(communities[i], active_nodes);
                if (static_cast<int>(active_nodes.size()) > c_min) {
                    active_nodes.erase(*communities[i].begin());
                }
            }
        }
    }
}

void HyPar::preprocess() {
    existing_nodes.clear();
    deleted_nodes.clear();
    for(size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
    int max_size = community_detect();
    int c_min = static_cast<int>(std::ceil(double(max_size) / (10)));
    int c_max = static_cast<int>(std::ceil(double(nodes.size()) / (K * parameter_t)));
    for (size_t i = 0; i < communities.size(); ++i) {
        if (static_cast<int>(communities[i].size()) <= c_min) {
            contract_in_community(i);  
        } else {
            fast_pin_sparsify_in_community(i, c_min, c_max);
        }
    }
    for (auto it = communities.begin(); it != communities.end();) {
        if (it->size() <= 1) {
            it = communities.erase(it);
        } else {
            ++it;
        }
    }
    std::cout << "After community contract: " << existing_nodes.size() << std::endl;
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
        } else {
            fast_pin_sparsify_in_community(i, c_min, c_max);
        }
    }
    communities.emplace_back(another_community);
    fast_pin_sparsify_in_community(communities.size() - 1, c_min, c_max);
}