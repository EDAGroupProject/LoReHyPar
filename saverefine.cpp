#include <fstream>
#include <sstream>
#include <iostream>
#include <hypar.hpp>

void HyPar::saveRefineResults(const std::string &filename) {
    std::ofstream outFile(filename, std::ios::binary);
    if (!outFile) {
        std::cerr << "Failed to open file for saving refine state: " << filename << std::endl;
        return;
    }

    // Save the contract memo stack
    size_t contractMemoSize = contract_memo.size();
    outFile.write(reinterpret_cast<const char *>(&contractMemoSize), sizeof(contractMemoSize));
    auto temp_stack = contract_memo;
    while (!temp_stack.empty()) {
        auto [u, v] = temp_stack.top();
        outFile.write(reinterpret_cast<const char *>(&u), sizeof(u));
        outFile.write(reinterpret_cast<const char *>(&v), sizeof(v));
        temp_stack.pop();
    }

    // Save the node states
    for (const auto& node : nodes) {
        outFile.write(reinterpret_cast<const char *>(&node.fpga), sizeof(node.fpga));
        size_t netsSize = node.nets.size();
        outFile.write(reinterpret_cast<const char *>(&netsSize), sizeof(netsSize));
        for (int net : node.nets) {
            outFile.write(reinterpret_cast<const char *>(&net), sizeof(net));
        }
    }

    outFile.close();
    std::cout << "Refine state saved to " << filename << std::endl;
}

void HyPar::loadRefineResults(const std::string &filename) {
    std::ifstream inFile(filename, std::ios::binary);
    if (!inFile) {
        std::cerr << "Failed to open file for loading refine state: " << filename << std::endl;
        return;
    }

    // Load the contract memo stack
    size_t contractMemoSize;
    inFile.read(reinterpret_cast<char *>(&contractMemoSize), sizeof(contractMemoSize));
    contract_memo = std::stack<std::pair<int, int>>();
    std::cout << "Loading contract memo stack (size: " << contractMemoSize << ")" << std::endl;
    for (size_t i = 0; i < contractMemoSize; ++i) {
        int u, v;
        inFile.read(reinterpret_cast<char *>(&u), sizeof(u));
        inFile.read(reinterpret_cast<char *>(&v), sizeof(v));
        contract_memo.push({u, v});
        std::cout << "Loaded contract pair: (" << u << ", " << v << ")" << std::endl;
    }

    // Load the node states
    for (auto& node : nodes) {
        inFile.read(reinterpret_cast<char *>(&node.fpga), sizeof(node.fpga));
        std::cout << "Loading node fpga: " << node.fpga << std::endl;
        size_t netsSize;
        inFile.read(reinterpret_cast<char *>(&netsSize), sizeof(netsSize));
        node.nets.clear();
        std::cout << "Loading node nets (size: " << netsSize << ")" << std::endl;
        for (size_t i = 0; i < netsSize; ++i) {
            int net;
            inFile.read(reinterpret_cast<char *>(&net), sizeof(net));
            node.nets.insert(net);
            std::cout << "Loaded net: " << net << std::endl;
        }
    }

    inFile.close();
    std::cout << "Refine state loaded from " << filename << std::endl;
}

void HyPar::saveRefineResultsAsText(const std::string &filename) {
    std::ofstream outFile(filename);
    if (!outFile) {
        std::cerr << "Failed to open file for saving refine state as text: " << filename << std::endl;
        return;
    }

    // Save the contract memo stack
    outFile << "Contract Memo Stack: " << contract_memo.size() << std::endl;
    auto temp_stack = contract_memo;
    while (!temp_stack.empty()) {
        auto [u, v] = temp_stack.top();
        outFile << u << " " << v << std::endl;
        temp_stack.pop();
    }

    // Save the node states
    outFile << "Node States: " << nodes.size() << std::endl;
    for (const auto& node : nodes) {
        outFile << "FPGA: " << node.fpga << " Nets: " << node.nets.size() << " ";
        for (int net : node.nets) {
            outFile << net << " ";
        }
        outFile << std::endl;
    }

    outFile.close();
    std::cout << "Refine state saved as text to " << filename << std::endl;
}

void HyPar::loadRefineResultsAsText(const std::string &filename) {
    std::ifstream inFile(filename);
    if (!inFile) {
        std::cerr << "Failed to open file for loading refine state as text: " << filename << std::endl;
        return;
    }

    std::string line;

    // Load the contract memo stack
    std::getline(inFile, line);
    if (line.empty() || line.find("Contract Memo Stack:") == std::string::npos) {
        std::cerr << "Error parsing contract memo stack size" << std::endl;
        return;
    }
    size_t contractMemoSize = 0;
    try {
        contractMemoSize = std::stoul(line.substr(line.find(":") + 1));
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error parsing contract memo stack size: " << e.what() << std::endl;
        return;
    }
    contract_memo = std::stack<std::pair<int, int>>();
    for (size_t i = 0; i < contractMemoSize; ++i) {
        std::getline(inFile, line);
        std::istringstream iss(line);
        int u, v;
        iss >> u >> v;
        contract_memo.push({u, v});
        std::cout << "Loaded contract pair: (" << u << ", " << v << ")" << std::endl;
    }

    // Load the node states
    std::getline(inFile, line);
    if (line.empty() || line.find("Node States:") == std::string::npos) {
        std::cerr << "Error parsing node states size" << std::endl;
        return;
    }
    size_t nodesSize = 0;
    try {
        nodesSize = std::stoul(line.substr(line.find(":") + 1));
    } catch (const std::invalid_argument &e) {
        std::cerr << "Error parsing node states size: " << e.what() << std::endl;
        return;
    }
    for (size_t i = 0; i < nodesSize; ++i) {
        std::getline(inFile, line);
        std::istringstream iss(line);
        std::string fpgaLabel;
        int fpga;
        size_t netsSize;
        iss >> fpgaLabel >> fpga >> fpgaLabel >> netsSize;
        nodes[i].fpga = fpga;
        nodes[i].nets.clear();
        std::cout << "Loading node " << i << " FPGA: " << fpga << " Nets (size: " << netsSize << ")" << std::endl;
        for (size_t j = 0; j < netsSize; ++j) {
            int net;
            iss >> net;
            nodes[i].nets.insert(net);
            std::cout << "Loaded net: " << net << std::endl;
        }
    }

    inFile.close();
    std::cout << "Refine state loaded from text file " << filename << std::endl;
}
