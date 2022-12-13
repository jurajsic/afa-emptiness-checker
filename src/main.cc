#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <mata/nfa.hh>
#include <mata/inter-aut.hh>
#include <mata/mintermization.hh>

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
    std::unordered_map<unsigned, Mata::Nfa::Nfa> num_to_aut;
    while(std::getline(input, line)) {
        line.erase(std::remove(line.begin(), line.end(), '('), line.end());
        line.erase(std::remove(line.begin(), line.end(), ')'), line.end());

        std::istringstream line_stream(line);
        std::string token;
        line_stream >> token;
        if (token == "load_automaton") {
            line_stream >> token;
            unsigned load_aut_num = get_aut_num(token);
            std::filesystem::path path_to_automaton = path_to_automata/(token + ".mata");
            std::ifstream aut_file(path_to_automaton);
            if (!aut_file.is_open()) {
                std::cerr << "ERROR: Could not open file " << path_to_automaton << std::endl;
                return -1;
            }

            Mata::Mintermization mintermization;
            auto inter_aut = mintermization.mintermize(Mata::IntermediateAut::parse_from_mf(Mata::Parser::parse_mf(aut_file, true))[0]);
            num_to_aut[load_aut_num] = Mata::Nfa::construct(inter_aut);
        } else if (token == "is_empty") {
            line_stream >> token;
            unsigned res_aut_num = get_aut_num(token);
            if (Mata::Nfa::is_lang_empty(num_to_aut.at(res_aut_num))) {
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
            line_stream >> token;
            unsigned operand_aut_num = get_aut_num(token);
            num_to_aut[res_aut_num] = num_to_aut.at(operand_aut_num);
            if (operation == "compl") {
                Mata::Nfa::complement_in_place(num_to_aut.at(res_aut_num));
            } else {
                while (line_stream >> token) {
                    operand_aut_num = get_aut_num(token);
                    if (operation == "union") {
                        num_to_aut[res_aut_num] = Mata::Nfa::uni(num_to_aut.at(res_aut_num), num_to_aut.at(operand_aut_num));
                    } else { //intersection
                        num_to_aut[res_aut_num] = Mata::Nfa::intersection(num_to_aut.at(res_aut_num), num_to_aut.at(operand_aut_num));
                    }
                }
            }
        }
    }
}