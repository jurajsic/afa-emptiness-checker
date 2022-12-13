#include <iostream>
#include <fstream>
#include <mata/nfa.hh>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "Program expects exactly one argument, try '--help' for help" << std::endl;
        return -1;
    }

    std::string arg = std::string(argv[1]);

    if (arg == "-h" || arg == "--help") {
        std::cout << "Usage:  emptiness-checker input.emp" << std::endl;
    }

    std::ifstream input(arg);
    if (!input.is_open()) {
        std::cerr << "Could not open file" << std::endl;
        return -1;
    }

    
}