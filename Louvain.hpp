#ifndef _LOUVAIN_HPP
#define _LOUVAIN__HPP

#include <hypar.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <numeric>

class Louvain {
    friend class HyPar;

    std::vector<std::unordered_map<int, float>> A;
    std::vector<int> community;
    std::vector<std::vector<int>> communityNodes;
    std::vector<float> k_tot;
    std::unordered_set<int> nodes;
    int n;
    float m;

    bool phaseOne();
    void phaseTwo();
    float calculateDeltaModularity(int u, int c);

public:
    Louvain(int _n);
    void addEdge(int u, int v, float w);
    void run(UnionFind &uf); // 修改run以接收并查集
    void print();
};

Louvain::Louvain(int _n) : n(_n), m(0.0) { 
    A.resize(n);
    community.resize(n);
    communityNodes.resize(n);
    k_tot.resize(n, 0.0);
    for (int i = 0; i < n; ++i) {
        community[i] = i;
        communityNodes[i].push_back(i);
    }
    nodes.insert(community.begin(), community.end());
}

void Louvain::addEdge(int u, int v, float w) {
    if (u >= n || v >= n) return;
    A[u][v] = w;
    A[v][u] = w;
    k_tot[u] += w;
    k_tot[v] += w;
    m += w;
}

void Louvain::run(UnionFind &uf) {
    int maxSize = 0;
    std::cout << "Union-Find (Connected Components):" << std::endl;

    for (int i = 0; i < n; ++i) {
        if (i < 0 || i >= int(uf.parent.size())) { // 添加边界检查，确保 i 在合法范围内
            std::cerr << "Warning: node index " << i << " is out of bounds for Union-Find." << std::endl;
            continue;
        }

        int root = uf.find(i);
        maxSize = std::max(maxSize, uf.getSize(root));
    }

    int threshold = maxSize / 2;

    // 将小于阈值的连通分量标记为固定社区
    for (int i = 0; i < n; ++i) {
        if (i < 0 || i >= uf.parent.size()) { // 再次检查 i 的范围
            continue;
        }
        int root = uf.find(i);
        if (uf.getSize(root) < threshold) {
            community[i] = root;
            communityNodes[root].push_back(i);
            nodes.erase(i); 
        }
    }

    // 对剩余节点执行 Louvain 算法
    while (phaseOne()) {
        phaseTwo();
    }
}


bool Louvain::phaseOne() {
    bool improvement = false;
    std::unordered_map<int, bool> is_community;
    for (int u : nodes) {
        if (is_community[u]) continue;

        int bestCommunity = community[u];
        float bestDeltaQ = 0.0;
        std::unordered_map<int, bool> community_caled;
        for (const auto& [v, w] : A[u]) {
            if (community_caled[community[v]]) continue;
            community_caled[community[v]] = true;
            float deltaQ = calculateDeltaModularity(u, community[v]);
            if (deltaQ > bestDeltaQ) {
                bestCommunity = community[v];
                bestDeltaQ = deltaQ;
            }
        }
        if (bestCommunity != community[u]) {
            k_tot[bestCommunity] += k_tot[u];
            community[u] = bestCommunity;
            communityNodes[bestCommunity].insert(communityNodes[bestCommunity].end(), communityNodes[u].begin(), communityNodes[u].end()); 
            is_community[bestCommunity] = true;
            improvement = true;
        }
    }
    return improvement;
}

void Louvain::phaseTwo() {
    for (auto it = nodes.begin(); it != nodes.end(); ) {
        int u = *it;
        if (community[u] == u) {
            for (auto ait = A[u].begin(); ait != A[u].end(); ) {
                int v = ait->first, w = ait->second;
                if (community[v] != v) {
                    ait = A[u].erase(ait);
                    if (community[v] != u) {
                        A[v][community[v]] += w;
                    }
                } else {
                    ++ait;
                }
            }
            ++it;
        } else {
            for (const auto& [v, w] : A[u]) {
                if (community[v] != community[u]) {
                    A[community[u]][community[v]] += w;
                }
            }
            it = nodes.erase(it);
        }
    }
}

float Louvain::calculateDeltaModularity(int u, int c) {
    float k_i_in = 0.0;
    for (const auto& [v, w] : A[u]) {
        if (community[v] == c) {
            k_i_in += w;
        }
    }
    return k_i_in / m - k_tot[u] * k_tot[c] / (2.0 * m * m);
}

void Louvain::print() {
    for (int c : nodes) {
        std::cout << "Community " << c << ": ";
        for (int u : communityNodes[c]) {
            std::cout << u << " ";
        }
        std::cout << std::endl;
    }
}

#endif // LOUVAIN_HPP
