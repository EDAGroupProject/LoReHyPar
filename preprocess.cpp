#include <hypar.hpp>
#include <LSHConstants.hpp>
#include <Louvain.hpp>
#include <cmath>
#include <algorithm>

void HyPar::pin_sparsify() {
    std::vector<int> nodeFp(nodes.size(),0);
    for (size_t i = 0; i < nodes.size(); ++i) {
        for (int net : nodes[i].nets) {
            nodeFp[i] += net * net;
        }
    }
    int hash_num = std::min(NUM_HASHES, static_cast<int>(std::ceil(std::sqrt(nodes.size()))));
    size_t c_min =static_cast<size_t>(std::sqrt(parameter_l)), c_max = static_cast<size_t>(std::ceil(float(nodes.size()) / (K * parameter_t))); // @warning: the range of c is arbitrary now
    std::unordered_set<int> active_nodes(existing_nodes);
    std::unordered_map<int, int> node2size;
    int hash_min = std::min(3, static_cast<int>(std::ceil(std::sqrt(hash_num)))), hash_max = static_cast<int>(std::ceil(double(hash_num * (K - 1)) / K));
    std::vector<int> hash_vec;
    for (int i = hash_min; i <= hash_max; ++i) {
        hash_vec.push_back(i);
    }
    std::mt19937 rng = get_rng();
    std::shuffle(hash_vec.begin(), hash_vec.end(), rng);
    std::uniform_int_distribution<int> dis(0, hash_num - 1);
    for (int hash_sel : hash_vec) {
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
            int fp = 0;
            for (int net : nodes[node].nets) {
                fp += net * net;
            }
            for (int i = 0; i < hash_sel; ++i) {
                int hash_id = slice[i];
                hashTable[node].emplace_back((A_VALUES[hash_id] * fp + B_VALUES[hash_id]) % P_VALUES[hash_id]);
            }
        }
        std::unordered_map<int, int> node2cluster;
        std::unordered_map<int, size_t> cluster2size;
        std::vector<std::vector<int>> clusters;
        std::vector<int> nodeVec(active_nodes.begin(), active_nodes.end());
        std::shuffle(nodeVec.begin(), nodeVec.end(), rng);
        for (int node : nodeVec) { // Merge neighbors with the same hash
            // if (node2cluster.count(node)) {
            //     continue;
            // }
            for (int net : nodes[node].nets) {
                for (int j = 0; j < nets[net].size; ++j) {
                    int neighbor = nets[net].nodes[j];
                    if (neighbor == node || !active_nodes.count(neighbor) || node2cluster.count(neighbor)) {
                        continue;
                    }
                    if (hashTable[node] == hashTable[neighbor]) {
                        if (!node2cluster.count(node)) {
                            node2cluster[node] = clusters.size();
                            clusters.push_back({node});
                            if (node2size.count(node)) {
                                cluster2size[clusters.size() - 1] = node2size[node];
                            } else {
                                cluster2size[clusters.size() - 1] = 1;
                            }
                        }
                        node2cluster[neighbor] = node2cluster[node];
                        clusters[node2cluster[node]].push_back(neighbor);
                        if (node2size.count(neighbor)) {
                            cluster2size[node2cluster[node]] += node2size[neighbor];
                        } else {
                            cluster2size[node2cluster[node]] += 1;
                        }
                    } 
                }
            }
        }
        for (size_t i = 0; i < clusters.size(); ++i) {
            size_t cluster_size = cluster2size[i];
            const auto &cluster = clusters[i];
            if (cluster_size > c_max || cluster_size == 1) {
                continue;
            }
            int u = cluster[0];
            for (size_t i = 1; i < cluster.size(); ++i) {
                int v = cluster[i];
                _contract(u, v);
                active_nodes.erase(v);
            }
            if (cluster_size < c_min) {
                node2size[u] = cluster_size;
            } else {
                active_nodes.erase(u); // the difference between the two branches
            }
        }
    }
}

void HyPar::community_detect() {
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
    lv.run(uf);
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
                node2community[node] = communities.size() - 1;
            }
        }
    }
}

void HyPar::preprocess() {
    Louvain lv(nets.size() + existing_nodes.size());
    existing_nodes.clear();
    deleted_nodes.clear();
    for(size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
    pin_sparsify();
    community_detect();
    lv.print();
}