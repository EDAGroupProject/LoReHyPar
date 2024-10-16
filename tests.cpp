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
    _test_simple_process(out);
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
    coarsen();
    out << "\nCoarsen: \n";
    printSummary(out);
    initial_partition();
    out << "\nInitial Partition: \n";
    printSummary(out);
    refine();
    out << "\nRefine: \n";
    printSummary(out);
    printOut(out);
}