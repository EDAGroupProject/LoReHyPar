#include <fstream>

#include <sstream>
#include <iostream>
#include <unordered_set>
#include <unordered_map>
#include <queue>
#include <random>
#include <climits>
#include <numeric>
#include <algorithm>
#include <hypar.hpp>


void HyPar::savePartitionResults(const std::string &filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for saving partition results: " << filename << std::endl;
        return;
    }

    // Save the existing_nodes set
    size_t existingNodesSize = existing_nodes.size();
    outFile.write(reinterpret_cast<const char *>(&existingNodesSize), sizeof(existingNodesSize));
    for (int node : existing_nodes) {
        outFile.write(reinterpret_cast<const char *>(&node), sizeof(node));
    }

    // Save the deleted_nodes set
    size_t deletedNodesSize = deleted_nodes.size();
    outFile.write(reinterpret_cast<const char *>(&deletedNodesSize), sizeof(deletedNodesSize));
    for (int node : deleted_nodes) {
        outFile.write(reinterpret_cast<const char *>(&node), sizeof(node));
    }

    // Save the communities vector
    size_t communitiesSize = communities.size();
    outFile.write(reinterpret_cast<const char *>(&communitiesSize), sizeof(communitiesSize));
    for (const auto &community : communities) {
        size_t communitySize = community.size();
        outFile.write(reinterpret_cast<const char *>(&communitySize), sizeof(communitySize));
        for (int node : community) {
            outFile.write(reinterpret_cast<const char *>(&node), sizeof(node));
        }
    }

    // Save the node2community map
    size_t node2CommunitySize = node2community.size();
    outFile.write(reinterpret_cast<const char *>(&node2CommunitySize), sizeof(node2CommunitySize));
    for (const auto &[node, community] : node2community) {
        outFile.write(reinterpret_cast<const char *>(&node), sizeof(node));
        outFile.write(reinterpret_cast<const char *>(&community), sizeof(community));
    }

    // Save the FPGA mapping and nodes
    size_t fpgasSize = fpgas.size();
    outFile.write(reinterpret_cast<const char *>(&fpgasSize), sizeof(fpgasSize));
    for (const auto &fpga : fpgas) {
        size_t nodesSize = fpga.nodes.size();
        outFile.write(reinterpret_cast<const char *>(&nodesSize), sizeof(nodesSize));
        for (int node : fpga.nodes) {
            outFile.write(reinterpret_cast<const char *>(&node), sizeof(node));
        }
    }

    outFile.close();
    std::cout << "Partition results saved to " << filename << std::endl;
}

void HyPar::loadPartitionResults(const std::string &filename) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        std::cerr << "Failed to open file for loading partition results: " << filename << std::endl;
        return;
    }

    // Load the existing_nodes set
    size_t existingNodesSize;
    inFile.read(reinterpret_cast<char *>(&existingNodesSize), sizeof(existingNodesSize));
    existing_nodes.clear();
    for (size_t i = 0; i < existingNodesSize; ++i) {
        int node;
        inFile.read(reinterpret_cast<char *>(&node), sizeof(node));
        existing_nodes.insert(node);
    }

    // Load the deleted_nodes set
    size_t deletedNodesSize;
    inFile.read(reinterpret_cast<char *>(&deletedNodesSize), sizeof(deletedNodesSize));
    deleted_nodes.clear();
    for (size_t i = 0; i < deletedNodesSize; ++i) {
        int node;
        inFile.read(reinterpret_cast<char *>(&node), sizeof(node));
        deleted_nodes.insert(node);
    }

    // Load the communities vector
    size_t communitiesSize;
    inFile.read(reinterpret_cast<char *>(&communitiesSize), sizeof(communitiesSize));
    communities.clear();
    for (size_t i = 0; i < communitiesSize; ++i) {
        size_t communitySize;
        inFile.read(reinterpret_cast<char *>(&communitySize), sizeof(communitySize));
        std::unordered_set<int> community;
        for (size_t j = 0; j < communitySize; ++j) {
            int node;
            inFile.read(reinterpret_cast<char *>(&node), sizeof(node));
            community.insert(node);
        }
        communities.push_back(community);
    }

    // Load the node2community map
    size_t node2CommunitySize;
    inFile.read(reinterpret_cast<char *>(&node2CommunitySize), sizeof(node2CommunitySize));
    node2community.clear();
    for (size_t i = 0; i < node2CommunitySize; ++i) {
        int node, community;
        inFile.read(reinterpret_cast<char *>(&node), sizeof(node));
        inFile.read(reinterpret_cast<char *>(&community), sizeof(community));
        node2community[node] = community;
    }

    // Load the FPGA mapping and nodes
    size_t fpgasSize;
    inFile.read(reinterpret_cast<char *>(&fpgasSize), sizeof(fpgasSize));
    fpgas.clear();
    fpgas.resize(fpgasSize);
    for (size_t i = 0; i < fpgasSize; ++i) {
        size_t nodesSize;
        inFile.read(reinterpret_cast<char *>(&nodesSize), sizeof(nodesSize));
        for (size_t j = 0; j < nodesSize; ++j) {
            int node;
            inFile.read(reinterpret_cast<char *>(&node), sizeof(node));
            fpgas[i].nodes.insert(node);
        }
    }

    inFile.close();
    std::cout << "Partition results loaded from " << filename << std::endl;
}

void HyPar::savePartitionResultsAsText(const std::string &filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Failed to open file for saving partition results: " << filename << std::endl;
        return;
    }

    // Save the existing_nodes set
    outFile << "Existing Nodes: " << existing_nodes.size() << std::endl;
    for (int node : existing_nodes) {
        outFile << node << " ";
    }
    outFile << std::endl;

    // Save the deleted_nodes set
    outFile << "Deleted Nodes: " << deleted_nodes.size() << std::endl;
    for (int node : deleted_nodes) {
        outFile << node << " ";
    }
    outFile << std::endl;

    // Save the communities vector
    outFile << "Communities: " << communities.size() << std::endl;
    for (size_t i = 0; i < communities.size(); ++i) {
        outFile << "Community " << i << " (Size: " << communities[i].size() << "): ";
        for (int node : communities[i]) {
            outFile << node << " ";
        }
        outFile << std::endl;
    }

    // Save the node2community map
    outFile << "Node to Community Map: " << node2community.size() << std::endl;
    for (const auto &[node, community] : node2community) {
        outFile << "Node " << node << " -> Community " << community << std::endl;
    }

    // Save the FPGA mapping and nodes
    outFile << "FPGAs: " << fpgas.size() << std::endl;
    for (size_t i = 0; i < fpgas.size(); ++i) {
        outFile << "FPGA " << i << " Nodes: ";
        for (int node : fpgas[i].nodes) {
            outFile << node << " ";
        }
        outFile << std::endl;
    }

    outFile.close();
    std::cout << "Partition results saved as text to " << filename << std::endl;
}

void HyPar::loadPartitionResultsAsText(const std::string &filename) {
    std::ifstream inFile(filename);
    if (!inFile) {
        std::cerr << "Failed to open file for loading partition results: " << filename << std::endl;
        return;
    }

    std::string line;
    existing_nodes.clear();
    deleted_nodes.clear();
    communities.clear();
    node2community.clear();
    fpgas.clear();

    // Load the existing_nodes set
    std::getline(inFile, line);
    size_t existingNodesSize = std::stoul(line.substr(line.find(":") + 1));
    if (existingNodesSize > 0) {
        std::getline(inFile, line);
        if (!line.empty()) {
            std::istringstream iss(line);
            for (size_t i = 0; i < existingNodesSize; ++i) {
                int node;
                iss >> node;
                existing_nodes.insert(node);
            }
        }
    }

    // Load the deleted_nodes set
    std::getline(inFile, line);
    size_t deletedNodesSize = std::stoul(line.substr(line.find(":") + 1));
    if (deletedNodesSize > 0) {
        std::getline(inFile, line);
        if (!line.empty()) {
            std::istringstream iss(line);
            for (size_t i = 0; i < deletedNodesSize; ++i) {
                int node;
                iss >> node;
                deleted_nodes.insert(node);
            }
        }
    }

    // Load the communities vector
    std::getline(inFile, line);
    size_t communitiesSize = std::stoul(line.substr(line.find(":") + 1));
    for (size_t i = 0; i < communitiesSize; ++i) {
        std::getline(inFile, line);
        if (!line.empty()) {
            std::istringstream iss(line.substr(line.find("): ") + 3));
            std::unordered_set<int> community;
            int node;
            while (iss >> node) {
                community.insert(node);
            }
            communities.push_back(community);
        }
    }

    // Load the node2community map
    std::getline(inFile, line);
    if (line.empty() || line.find("Node to Community Map:") == std::string::npos) {
        std::cerr << "Error parsing node2CommunitySize" << std::endl;
        return;
    }
    size_t node2CommunitySize = 0;
    try {
        node2CommunitySize = std::stoul(line.substr(line.find(":") + 1));
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error parsing node2CommunitySize: " << e.what() << std::endl;
        return;
    }
    for (size_t i = 0; i < node2CommunitySize; ++i) {
        std::getline(inFile, line);
        std::cout << "node2CommunitySize line: " << line << std::endl;
        if (!line.empty() && line.find("Node ") == 0) {
            int node = 0, community = 0;
            try {
                size_t nodeStart = 5; // "Node " 后面的第一个字符位置
                size_t nodeEnd = line.find(" ->"); // " ->" 的起始位置
                size_t communityStart = line.find("Community ") + 10; // "Community " 后面的第一个字符位置

                std::string nodeStr = line.substr(nodeStart, nodeEnd - nodeStart);
                std::string communityStr = line.substr(communityStart);

                std::cout << "nodeStr: " << nodeStr << ", communityStr: " << communityStr << std::endl;

                node = std::stoi(nodeStr);
                community = std::stoi(communityStr);
            } catch (const std::invalid_argument &e) {
                std::cerr << "Error parsing node to community mapping: " << e.what() << std::endl;
                return;
            }
            node2community[node] = community;
        }
    }

    // Load the FPGA mapping and nodes
    std::getline(inFile, line);
    size_t fpgasSize = std::stoul(line.substr(line.find(":") + 1));
    fpgas.resize(fpgasSize);
    for (size_t i = 0; i < fpgasSize; ++i) {
        std::getline(inFile, line);
        if (!line.empty()) {
            std::istringstream iss(line.substr(line.find("Nodes: ") + 7));
            int node;
            std::cout << "FPGA " << i << " nodes: ";
            while (iss >> node) {
                fpgas[i].nodes.insert(node);
                nodes[node].fpga = i; // Update the FPGA value of the node
                std::cout << node << " ";
            }
            std::cout << std::endl;
        }
    }

    // Output the FPGA values of nodes after loading
    std::cout << "Nodes' FPGA values after loading partition results:" << std::endl;
    for (size_t i = 0; i < nodes.size(); ++i) {
        std::cout << "Node " << i << ": FPGA " << nodes[i].fpga << std::endl;
    }

    inFile.close();
    std::cout << "Partition results loaded from text file " << filename << std::endl;
}