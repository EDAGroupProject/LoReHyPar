#include <hypar.hpp>
#include <LSHConstants.hpp>
#include <Louvain.hpp>
#include <cmath>
#include <algorithm>

bool vector_equal(const std::vector<int> slice, const std::vector<int> &vec1, const std::vector<int> &vec2){
    for (int i : slice){
        if (vec1[i] != vec2[i]){
            return false;
        }
    }
    return true;
}

void HyPar::pin_sparsify(){
    std::vector<int> nodeFp(nodes.size(),0);
    for (size_t i = 0; i < nodes.size(); ++i){
        for (int net : nodes[i].nets){
            nodeFp[i] += net * net;
        }
    }
    int hash_num = std::min(NUM_HASHES, static_cast<int>(std::ceil(std::log(nodes.size()) / std::log(2))));
    std::vector<std::vector<int>> hashTable(nodes.size(), std::vector<int>(hash_num));
    for (size_t i = 0; i < nodes.size(); ++i){
        for (int j = 0; j < hash_num; ++j){
            hashTable[i][j] = (A_VALUES[j] * nodeFp[i] + B_VALUES[j]) % P_VALUES[j];
        }
    }
    size_t c_min =static_cast<size_t>(std::sqrt(K)), c_max = static_cast<size_t>(float(nodes.size()) / (K * parameter_t)); // @warning: the range of c is arbitrary now
    std::unordered_set<int> active_nodes(existing_nodes);
    for (int hash_sel = std::max(1, static_cast<int>(std::ceil(std::log(hash_num) / std::log(10)))); hash_sel <= hash_num; ++hash_sel){
        std::vector<int> slice(hash_sel);
        std::mt19937 rng = get_rng();
        std::uniform_int_distribution<int> dis(0, hash_num - 1);
        for (int i = 0; i < hash_sel; ++i){
            int rd = dis(rng);
            while (std::find(slice.begin(), slice.end(), rd) != slice.end()){
                rd = dis(rng);
            }
            slice[i] = rd;
        }
        std::unordered_map<int, int> node2cluster;
        std::vector<std::vector<int>> clusters;
        for (int node : active_nodes) {
            if (node2cluster.count(node)){
                continue;
            }
            for (int net : nodes[node].nets){
                for (int neighbor : nets[net].nodes){
                    if (neighbor == node || !active_nodes.count(neighbor) || node2cluster.count(neighbor)){
                        continue;
                    }
                    if (vector_equal(slice, hashTable[node], hashTable[neighbor])){ // ensured by the order
                        if (!node2cluster.count(node)){
                            node2cluster[node] = clusters.size();
                            clusters.push_back({node});
                        }
                        node2cluster[neighbor] = node2cluster[node];
                        clusters[node2cluster[node]].push_back(neighbor);
                    } 
                }
            }
        }
        for (auto &cluster : clusters){
            if (cluster.size() > c_max || cluster.size() == 1){
                continue;
            }
            if (cluster.size() < c_min){
                int u = cluster[0];
                for (size_t i = 1; i < cluster.size(); ++i){
                    int v = cluster[i];
                    _contract(u, v);
                    active_nodes.erase(v);
                    std::transform(hashTable[v].begin(), hashTable[v].end(), hashTable[u].begin(), hashTable[u].begin(), std::plus<int>());
                }
                std::transform(hashTable[u].begin(), hashTable[u].end(), hashTable[u].begin(), [hash_sel](int a){return a % P_VALUES[hash_sel];});
            } else {
                int u = cluster[0];
                active_nodes.erase(u);
                for (size_t i = 1; i < cluster.size(); ++i){
                    int v = cluster[i];
                    _contract(u, v);
                    active_nodes.erase(v);
                }
            }
        }
    }
}

void HyPar::community_detect(){
    Louvain lv(nets.size() + existing_nodes.size());
    std::vector<int> louvain2node;
    float edge_density = float(nets.size()) / nodes.size();
    if (edge_density >= 0.75){
        for (int node : existing_nodes){
            louvain2node.push_back(node);
            for (int net : nodes[node].nets){
                lv.addEdge(nets.size() + node, net, 1.0);
            }
        }
    } else {
        for (int node : existing_nodes){
            louvain2node.push_back(node);
            size_t d = nodes[node].nets.size();
            for (int net : nodes[node].nets){
                lv.addEdge(nets.size() + node, net, float(d) / nets[net].size);
            }
        }
    }
    lv.run();
    for (int c : lv.nodes){
        commuinities.push_back({});
        for (int u : lv.communityNodes[c]){
            if (u >= nets.size()){
                int node = louvain2node[u - nets.size()];
                commuinities.back().insert(node);
                node2community[node] = commuinities.size() - 1;
            }
        }
    }
}

void HyPar::preprocess(){
    existing_nodes.clear();
    deleted_nodes.clear();
    for(size_t i = 0; i < nodes.size(); ++i){
        existing_nodes.insert(i);
    }
    pin_sparsify();
    community_detect();
}