#ifndef _LOUVAIN_HPP
#define _LOUVAIN__HPP

#include <hypar.hpp>
#include <vector>
#include <unordered_set>
#include <unordered_map>
#include <cmath>

class Louvain {
    friend class HyPar;

    std::vector<std::unordered_map<int, double>> A;
    std::vector<int> community;
    std::vector<std::vector<int>> communityNodes;
    std::vector<double> k_tot;
    std::unordered_set<int> nodes;
    int n;
    double m;

    bool phaseOne();
    void phaseTwo();
    double calculateDeltaModularity(int u, int c);

public:
    Louvain(int _n);
    void addEdge(int u, int v, double w);
    void run();
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

void Louvain::addEdge(int u, int v, double w) {
    if (u >= n || v >= n) return;
    A[u][v] = w;
    A[v][u] = w;
    k_tot[u] += w;
    k_tot[v] += w;
    m += w;
}

bool Louvain::phaseOne() {
    bool improvement = false;
    std::unordered_map<int, bool> is_community;
    for (int u : nodes) {
        if (is_community[u]) { // wont merge a community to another community
            continue;
        }
        int bestCommunity = community[u];
        double bestDeltaQ = 0.0;
        std::unordered_map<int, bool> community_caled;
        for (const auto& [v, w] : A[u]) {
            if (community_caled[community[v]]) continue;
            community_caled[community[v]] = true;
            double deltaQ = calculateDeltaModularity(u, community[v]);
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
        if (community[u] == u) { // original community node
            for (auto ait = A[u].begin(); ait != A[u].end(); ) {
                int v = ait->first, w = ait->second;
                if (community[v] != v) { // not a community node
                    ait = A[u].erase(ait);
                    if (community[v] != u) { // not a internal node, relink the edge
                        A[v][community[v]] += w;
                    }
                } else {
                    ++ait;
                }
            }
            ++it;
        } else{
            for (const auto& [v, w] : A[u]) {
                if (community[v] != community[u]) { // calulated once for u, the second time for v
                    A[community[u]][community[v]] += w;
                    // k_tot[community[u]] += w; // unnecessary, calculated in phaseOne
                }
            }
            it = nodes.erase(it);
        }
    }
}

void Louvain::run() {
    while (phaseOne()) {
        phaseTwo();
    }
}

double Louvain::calculateDeltaModularity(int u, int c) {
    double k_i_in = 0.0;
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
