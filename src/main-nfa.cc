#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>

#include <mata/nfa.hh>
#include <mata/inter-aut.hh>
#include <mata/mintermization.hh>

// TODO: set this by arguments?
// the size of the automaton should be reduced after loading it, or when it is assigned the result of operation
const bool REDUCE_SIZE_OF_RESULT = true;

// during computing union/intersection of automata, the result is reduced after 
const bool REDUCE_SIZE_AFTER_OPERATION = true;

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
        std::cout << "Usage: nfa-emptiness-checker input.emp" << std::endl;
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
    std::unordered_map<unsigned, std::istringstream> num_to_operation;
    std::vector<unsigned> input_aut_nums;
    std::vector<Mata::IntermediateAut> input_aut_inter_auts;
    unsigned aut_to_test_emptiness;
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
            input_aut_nums.push_back(load_aut_num);
            std::stringstream aut_with_split_transitions;
            std::string line;
            while(std::getline(aut_file, line)) {
                if (!line.empty() && line[0] == 'q' && line.back() == ')') {
                    std::string state_from_and_symbols = line.substr(0, line.find_last_of('('));
                    std::string states_to = line.substr(line.find_last_of('('));
                    std::string chars_to_remove = "()|";
                    states_to.erase(std::remove_if(states_to.begin(), states_to.end(), [&chars_to_remove](unsigned char c) { return chars_to_remove.find(c) != std::string::npos; } ), states_to.end());
                    std::stringstream ss(states_to);
                    std::string state_to;
                    while (ss >> state_to) {
                        aut_with_split_transitions << state_from_and_symbols << state_to << std::endl;
                    }
                } else {
                    aut_with_split_transitions << line << std::endl;
                }
            }
            input_aut_inter_auts.push_back(Mata::IntermediateAut::parse_from_mf(Mata::Parser::parse_mf(aut_with_split_transitions, true))[0]);
        } else if (token == "is_empty") {
            line_stream >> token;
            aut_to_test_emptiness = get_aut_num(token);
        } else {
            unsigned res_aut_num = get_aut_num(token);
            line_stream >> token; //'='

            num_to_operation[res_aut_num] = std::move(line_stream);
        }
    }


    std::cerr << "Starting mintermization" << std::endl;
    auto start = std::chrono::steady_clock::now();
    Mata::Mintermization mintermization;
    auto mintermized_input_inter_auts = mintermization.mintermize(input_aut_inter_auts);
    input_aut_inter_auts.clear();
    auto end = std::chrono::steady_clock::now();
    std::chrono::duration<double> elapsed_seconds = end-start;
    std::cerr << "Time of mintermization: " << elapsed_seconds.count() << std::endl;

    std::unordered_map<unsigned, Mata::Nfa::Nfa> num_to_aut;
    Mata::Nfa::OnTheFlyAlphabet alphabet;
    for (unsigned i = 0; i < mintermized_input_inter_auts.size(); ++i) {
        if (REDUCE_SIZE_OF_RESULT) {
            num_to_aut[input_aut_nums[i]] = Mata::Nfa::reduce(Mata::Nfa::construct(mintermized_input_inter_auts[i], &alphabet));
        } else {
            num_to_aut[input_aut_nums[i]] = Mata::Nfa::construct(mintermized_input_inter_auts[i], &alphabet);
        }
    }

    std::function<Mata::Nfa::Nfa& (unsigned)> getAutFromNum;
    getAutFromNum = [&num_to_aut, &num_to_operation, &getAutFromNum, &alphabet](unsigned aut_num) -> Mata::Nfa::Nfa& {
        auto it = num_to_aut.find(aut_num);
        if (it != num_to_aut.end()) {
            return it->second;
        }


        std::istringstream &operation_string_stream = num_to_operation.at(aut_num);
        std::string operation;
        operation_string_stream >> operation;
        std::string token;
        operation_string_stream >> token;
        unsigned first_operand_num = get_aut_num(token);
        Mata::Nfa::Nfa result;

        if (operation == "compl") {
            // TODO add minimization into the determinization which is done during the complement (and remove sink states of complement automaton)
            result = Mata::Nfa::complement(getAutFromNum(first_operand_num), alphabet);
            // TODO remove this after we add minimization to complement
            result = Mata::Nfa::reduce(result);
        } else {
            while (operation_string_stream >> token) {
                Mata::Nfa::Nfa &operand_nfa = getAutFromNum(get_aut_num(token));
                // TODO: maybe add operation and/union of multiple automata to mata
                if (operation == "union") {
                    // TODO union in place add here after it is added in mata and reduce at end
                    result = Mata::Nfa::uni(result, operand_nfa);
                } else { //intersection
                    // TODO reduce only here after union in place is added
                    result = Mata::Nfa::intersection(result, operand_nfa);
                }
                if (REDUCE_SIZE_AFTER_OPERATION) {
                    result = Mata::Nfa::reduce(result);
                }
            }
            if (REDUCE_SIZE_OF_RESULT && !REDUCE_SIZE_AFTER_OPERATION) {
                result = Mata::Nfa::reduce(result);
            }
        }

        num_to_aut[aut_num] = std::move(result);
        return num_to_aut.at(aut_num);
    };

    if (Mata::Nfa::is_lang_empty(getAutFromNum(aut_to_test_emptiness))) {
        std::cout << "EMPTY" << std::endl;
        return 0;
    } else {
        std::cout << "NOT EMPTY" << std::endl;
        return 0;
    }
}