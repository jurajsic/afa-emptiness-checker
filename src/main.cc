#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <sstream>
#include <vector>

unsigned get_aut_num(std::string aut_string) {
    if (aut_string.compare(0, 3, "aut") == 0) {
        return std::stoul(aut_string.substr(3));
    } else {
        std::cerr << "Expecting autN, something else was found" << std::endl;
        exit(-1);
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "ERROR: Program expects exactly one argument, try '--help' for help" << std::endl;
        return -1;
    }

    std::string arg = std::string(argv[1]);

    if (arg == "-h" || arg == "--help") {
        std::cout << "Usage: Cemptiness-checker input.emp" << std::endl;
        return 0;
    }

    std::filesystem::path path_to_input(arg);
    std::filesystem::path path_to_automata = std::filesystem::absolute(path_to_input).parent_path()/"gen_aut";

    std::ifstream input(path_to_input);
    if (!input.is_open()) {
        std::cerr << "ERROR: could not open file " << path_to_input << std::endl;
        return -1;
    }

    std::string line;
    // CHANGE THIS: void* should be pointer to mona automaton
    std::unordered_map<unsigned, void*> num_to_aut;
    while(std::getline(input, line)) {
        line.erase(std::remove(line.begin(), line.end(), '('), line.end());
        line.erase(std::remove(line.begin(), line.end(), ')'), line.end());

        std::istringstream line_stream(line);
        std::string token;
        line_stream >> token;
        if (token == "load_automaton") {
            line_stream >> token;
            unsigned load_aut_num = get_aut_num(token);
            std::filesystem::path path_to_automaton = path_to_automata/(token + "_mona.mata");
            std::ifstream aut_file(path_to_automaton);
            if (!aut_file.is_open()) {
                std::cerr << "ERROR: Could not open file " << path_to_automaton << std::endl;
                return -1;
            }
            // CHANGE THIS: this should load the automaton from file
            num_to_aut[load_aut_num] = load_automaton(aut_file);
        } else if (token == "is_empty") {
            line_stream >> token;
            unsigned res_aut_num = get_aut_num(token);
            // CHANGE THIS: test emptiness of automaton
            if (test_emptiness_of_automaton(num_to_aut[res_aut_num])) {
                std::cout << "EMPTY" << std::endl;
                return 0;
            } else {
                std::cout << "NOT EMPTY" << std::endl;
                return 0;
            }
        } else {
            unsigned res_aut_num = get_aut_num(token);
            line_stream >> token; //'='
            std::string operation;
            line_stream >> operation;

            // CHANGE THIS: void* should be pointer to mona automaton
            std::vector<void*> operands;
            while (line_stream >> token) {
                operands.push_back(num_to_aut.at(get_aut_num(token)));
            }

            if (operands.size() == 0) {
                std::cerr << "No operand for some operation" << std::endl;
                return -1;
            }

            if (operation == "compl") {
                if (operands.size() != 1) {
                    std::cerr << "Complementing can be done with only one automaton" << std::endl;
                }
                // CHANGE THIS: get complement of mona automaton
                num_to_aut[res_aut_num] = get_complement_of_automaton(operands[0]);
            } else {
                if (operation == "union") {
                    // CHANGE THIS: get union of automata saved in vector
                    num_to_aut[res_aut_num] = get_union_of_automata(operands);
                } else { //intersection
                    // CHANGE THIS: get intersection of automata saved in vector
                    num_to_aut[res_aut_num] = get_intersection_of_automata(operands);
                }
            }
        }
    }
}