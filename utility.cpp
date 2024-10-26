#include <hypar.hpp>

void ParFunc::run(){
    coarsen();
    initial_partition();
    refine();
    std::ofstream out(outputFile);
    printOut(out);
}

void ParFunc::test(std::string testOutput){
    std::ofstream out(testOutput);
    // _test_contract(out);
    // _test_simple_process(out);
    _test_k_local_search(out);
}

void ParFunc::evaluate(){
    for (auto &fpga : fpgas){
        if (fpga.resValid && fpga.conn <= fpga.maxConn){
            std::cout << "Valid FPGA: " << fpga.name << std::endl;
        } else {
            std::cerr << "Invalid FPGA: " << fpga.name << std::endl;
            for (int i = 0; i < NUM_RES; ++i){
                if (fpga.resUsed[i] > fpga.resCap[i]){
                    std::cerr << "Resource " << i << " exceeds the capacity: " << fpga.resUsed[i] << " > " << fpga.resCap[i] << std::endl;
                }
            }
            if (fpga.conn > fpga.maxConn){
                std::cerr << "Connection exceeds the capacity: " << fpga.conn << " > " << fpga.maxConn << std::endl;
            }
        }
    }
    int totalHop = 0;
    for (auto &net : nets){
        int source = net.source;
        int sf = nodes[source].fpga;
        for (int i = 0; i < net.size; ++i){
            int v = net.nodes[i];
            if (v == source){
                continue;
            }
            int vf = nodes[v].fpga;
            if (fpgaMap[sf][vf] == -1){
                std::cerr << "Errors happens between " << nodes[source].name << " and " << nodes[v].name <<"  No path between " << sf << " and " << vf << std::endl;
            } else {
                totalHop += net.weight * fpgaMap[sf][vf]; 
            }   
        }
    }
    std::cout << "Total Hop: " << totalHop << std::endl;
}

void ParFunc::_test_contract(std::ofstream &out){
    int u = 0;
    for (size_t v = 1; v < nodes.size(); ++v){
        _contract(u, v);
        out << "\nContract " << u << " " << v << std::endl;
        printSummary(out);
    }
    while (!contMeme.empty()){
        auto [u, v] = contMeme.top();
        _uncontract(u, v);
        out << "\nUncontract " << u << " " << v << std::endl;
        printSummary(out);
    }
}

void ParFunc::_test_simple_process(std::ofstream &out){
    out << "Original: \n";
    printSummary(out);
    coarsen_naive();
    out << "\nCoarsen: \n";
    printSummary(out);
    bfs_partition();
    out << "\nInitial Partition: \n";
    printSummary(out);
    refine_naive();
    out << "\nRefine: \n";
    printSummary(out);
    printOut(out);
}

void ParFunc::_test_k_local_search(std::ofstream &out){
    out << "Original: \n";
    printSummary(out);
    coarsen_naive();
    out << "\nCoarsen: \n";
    printSummary(out);
    bfs_partition();
    out << "\nInitial Partition: \n";
    printSummary(out);
    refine();
    out << "\nRefine: \n";
    printSummary(out);
    printOut(out);
}