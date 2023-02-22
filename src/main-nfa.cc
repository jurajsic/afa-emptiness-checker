#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <exception>
#include <sstream>

#include <mata/nfa.hh>
#include <mata/inter-aut.hh>
#include <mata/mintermization.hh>

#include <vata/explicit_tree_aut.hh>
#include <vata/parsing/timbuk_parser.hh>

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

VATA::ExplicitTreeAut mataToVata(const Mata::Nfa::Nfa &nfa, const std::string &header, VATA::Parsing::TimbukParser &parser) {
    std::stringstream ss;
    ss << header;
    ss << "States";
    for (Mata::Nfa::State s = 0; s < nfa.size(); ++s) {
        ss << " q" << s;
    }
    ss << std::endl << "Final States";
    for (Mata::Nfa::State s : nfa.final.get_elements()) {
        ss << " q" << s;
    }
    ss << std::endl << "Transitions" << std::endl;
    for (Mata::Nfa::State s : nfa.initial.get_elements()) {
        ss << "x -> q" << s << std::endl;
    }
    for (const auto &tran : nfa.delta) {
        ss << "a" << tran.symb << "(q" << tran.src << ") -> q" << tran.tgt << std::endl;
    }
    VATA::ExplicitTreeAut result;
    result.LoadFromString(parser, ss.str());
    return result;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "error: Program expects exactly one argument, try '--help' for help" << std::endl;
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
        std::cerr << "error: could not open file " << path_to_input << std::endl;
        return -1;
    }

    try {
        std::string line;
        std::unordered_map<unsigned, std::istringstream> num_to_operation;
        std::vector<unsigned> input_aut_nums;
        std::vector<Mata::IntermediateAut> input_aut_inter_auts;
        unsigned aut_to_test_emptiness = -1;
        bool test_inclusion = false;
        unsigned aut_to_test_incl1 = -1;
        unsigned aut_to_test_incl2 = -1;
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
                    std::cerr << "error: Could not open file " << path_to_automaton << std::endl;
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
            } else if (token == "incl") {
                line_stream >> token;
                aut_to_test_incl1 = get_aut_num(token);
                line_stream >> token;
                aut_to_test_incl2 = get_aut_num(token);
                test_inclusion = true;
            } else {
                unsigned res_aut_num = get_aut_num(token);
                line_stream >> token; //'='

                num_to_operation[res_aut_num] = std::move(line_stream);
            }
        }

        auto start = std::chrono::steady_clock::now();
        Mata::Mintermization mintermization;
        auto mintermized_input_inter_auts = mintermization.mintermize(input_aut_inter_auts);
        input_aut_inter_auts.clear();
        auto end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::cerr << "mintermization: " << elapsed_seconds.count() << std::endl;
        std::cout << "mintermization: " << elapsed_seconds.count() << std::endl;

        std::unordered_map<unsigned, VATA::ExplicitTreeAut> num_to_aut;
        VATA::Parsing::TimbukParser parser;
        Mata::OnTheFlyAlphabet alphabet;
        std::vector<Mata::Nfa::Nfa> mintermized_nfas;
        for (unsigned i = 0; i < mintermized_input_inter_auts.size(); ++i) {
            mintermized_nfas.push_back(Mata::Nfa::construct(mintermized_input_inter_auts[i], &alphabet));
        }

        std::stringstream ss;
        ss << "Ops";
        for (Mata::Symbol s : alphabet.get_alphabet_symbols()) {
            ss << " a" << s << ":1";
        }
        ss << " x:0" << std::endl << std::endl << "Automaton A" << std::endl;

        for (unsigned i = 0; i < mintermized_input_inter_auts.size(); ++i) {
            if (REDUCE_SIZE_OF_RESULT) {
                num_to_aut[input_aut_nums[i]] = mataToVata(mintermized_nfas[i], ss.str(), parser).Reduce();
            } else {
                num_to_aut[input_aut_nums[i]] = mataToVata(mintermized_nfas[i], ss.str(), parser);
            }
        }

        std::function<VATA::ExplicitTreeAut& (unsigned)> getAutFromNum;
        getAutFromNum = [&num_to_aut, &num_to_operation, &getAutFromNum, &alphabet](unsigned aut_num) -> VATA::ExplicitTreeAut& {
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
            VATA::ExplicitTreeAut result;

            if (operation == "compl") {
                result = getAutFromNum(first_operand_num).Complement();
            } else {
                result = getAutFromNum(first_operand_num);
                while (operation_string_stream >> token) {
                    VATA::ExplicitTreeAut &operand_nfa = getAutFromNum(get_aut_num(token));
                    // TODO: maybe add operation and/union of multiple automata to mata
                    if (operation == "union") {
                        result = VATA::ExplicitTreeAut::Union(result, operand_nfa);
                    } else { //intersection
                        result = VATA::ExplicitTreeAut::Intersection(result, operand_nfa);
                    }
                    if (REDUCE_SIZE_AFTER_OPERATION) {
                        result = result.Reduce();
                    }
                }
                if (REDUCE_SIZE_OF_RESULT && !REDUCE_SIZE_AFTER_OPERATION) {
                    result = result.Reduce();
                }
            }

            num_to_aut[aut_num] = std::move(result);
            return num_to_aut.at(aut_num);
        };

        if (test_inclusion) {
            VATA::InclParam inclParams;
            inclParams.SetAlgorithm(VATA::InclParam::e_algorithm::antichains);
            inclParams.SetDirection(VATA::InclParam::e_direction::upward);
            if (VATA::ExplicitTreeAut::CheckInclusion(getAutFromNum(aut_to_test_incl1), getAutFromNum(aut_to_test_incl2), inclParams)) {
                std::cout << "result: EMPTY" << std::endl;
                return 0;
            } else {
                std::cout << "result: NOT EMPTY" << std::endl;
                return 0;
            }
        } else {
            if (getAutFromNum(aut_to_test_emptiness).IsLangEmpty()) {
                std::cout << "result: EMPTY" << std::endl;
                return 0;
            } else {
                std::cout << "result: NOT EMPTY" << std::endl;
                return 0;
            }
        }

    } catch (const std::exception &exc) {
        std::cerr << "error: " << exc.what();
        return -1;
    }
}