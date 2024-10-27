#ifndef _LOUVAIN_HPP
#define _LOUVAIN__HPP

#include <hypar.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cmath>

class Louvain {
    friend class HyPar;

    std::vector<std::unordered_map<int, float, pair_hash>> A;
    std::vector<int> community;
    std::vector<std::vector<int>> communityNodes;
    std::vector<float> k_in, k_tot;
    std::unordered_set<int> nodes;
    int n;
    float m;

    bool phaseOne();
    void phaseTwo();
    void updateCommunity(int u, int c);
    float calculateDeltaModularity(int u, int c);

public:
    Louvain(int _n);
    void addEdge(int u, int v, float w);
    void run();
};

Louvain::Louvain(int _n) : n(_n), m(0.0) { 
    A.resize(n);
    community.resize(n);
    communityNodes.resize(n);
    k_in.resize(n, 0.0);
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
    m += w / 2.0;
}

bool Louvain::phaseOne() {
    bool improvement = false;
    k_in.assign(n, 0.0);
    std::unordered_map<int, bool> is_community;
    for (int u : nodes) {
        if (is_community[u]) { // wont merge a community to another community
            continue;
        }
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
            updateCommunity(u, bestCommunity);
            is_community[bestCommunity] = true;
            improvement = true;
        }
    }
    return improvement;
}

void Louvain::phaseTwo(){
    for (int u : nodes) {
        if (community[u] == u) { // initialize the community
            A[u].clear();
            k_in[u] = 0.0;
            k_tot[u] = 0.0;
        }
    }
    for (auto it = nodes.begin(); it != nodes.end(); ) {
        int u = *it;
        if (community[u] == u) {
            ++it;
            continue;
        }
        for (const auto& [v, w] : A[u]) {
            if (community[v] != community[u]) { // calulated once for u, the second time for v
                A[community[u]][community[v]] += w;
                k_tot[community[u]] += w;
            }
        }
        it = nodes.erase(it);
    }
}

void Louvain::run() {
    while (phaseOne()) {
        phaseTwo();
    }
}

float Louvain::calculateDeltaModularity(int u, int c) {
    float k_i_in = 0.0;
    for (const auto& [v, w] : A[u]) {
        if (community[v] == c) {
            k_i_in += w;
        }
    }
    return k_i_in / (2.0 * m) - k_tot[u] * k_in[c] / (2.0 * m * m);
}

void Louvain::updateCommunity(int u, int c) {
    for (const auto& [v, w] : A[u]) {
        if (community[v] == c) {
            k_in[c] += w;
        }
    }
    k_tot[c] += k_tot[u];
    community[u] = c;
    communityNodes[c].insert(communityNodes[c].end(), communityNodes[u].begin(), communityNodes[u].end()); // concatenate
}

#endif // LOUVAIN_HPP
