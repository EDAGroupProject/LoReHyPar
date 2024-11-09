#include <hypar.hpp>
#include <Louvain.hpp>
#include <cmath>
#include <algorithm>

void HyPar::community_detect() {
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
    int cnt = -1;
    for (int c : lv.nodes) {
        bool flag = false;
        for (int u : lv.communityNodes[c]) {
            if (u >= static_cast<int>(nets.size())) {
                if (!flag) {
                    flag = true;
                    ++cnt;
                }
                int node = louvain2node[u - nets.size()];
                node2community[node] = cnt;
            }
        }
    }
}

void HyPar::preprocess() {
    for(size_t i = 0; i < nodes.size(); ++i) {
        existing_nodes.insert(i);
    }
    community_detect();
}
