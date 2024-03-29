#include <iostream>
#include <fstream>
#include <filesystem>
#include <unordered_map>
#include <chrono>
#include <exception>

#include <mata/nfa.hh>
#include <mata/inter-aut.hh>
#include <mata/mintermization.hh>


unsigned get_aut_num(std::string aut_string) {
    if (aut_string.compare(0, 3, "aut") == 0) {
        return std::stoul(aut_string.substr(3));
    } else {
        throw std::runtime_error("Expecting autN, something else was found");
    }
}

int main(int argc, char** argv) {
    try {
        std::string input_emp_name;
        
        // the size of the automaton should be reduced when it is assigned the result of operation
        bool REDUCE_SIZE_OF_RESULT = true;
        // the size of the automaton should be reduced after loading it
        bool REDUCE_SIZE_OF_INPUT = true;
        // during computing union/intersection of automata, the result is reduced after 
        bool REDUCE_SIZE_AFTER_OPERATION = true;

        bool use_antichain_incl = true;

        bool minimize_complement = false;

        for (int arg_i = 1; arg_i < argc; ++arg_i) {
            std::string arg = std::string(argv[arg_i]);

            if (arg == "-h" || arg == "--help") {
                std::cout << "Usage: nfa-emptiness-checker [--no-operation-reduce] [--no-result-reduce] [--simple-inclusion] [--minimize-complement] input.emp" << std::endl << std::endl;
                std::cout << "            --no-operation-reduce     do not reduce automaton after each intersection/union, only after all" << std::endl;
                std::cout << "            --no-result-reduce        do not reduce automaton on the final result of intersection/union/complement" << std::endl;
                std::cout << "            --simple-inclusion        do not use antichains for inclusion" << std::endl;
                std::cout << "            --minimize-complement     minimize the deterministic automaton computed in complement (if it is not set, reduction happens based on the other parameters)" << std::endl;
                return 0;
            } else if (arg == "--no-operation-reduce") {
                REDUCE_SIZE_AFTER_OPERATION = false;
            } else if (arg == "--no-result-reduce") {
                REDUCE_SIZE_OF_RESULT = false;
            } else if (arg == "--simple-inclusion") {
                use_antichain_incl = false;
            } else if (arg == "--minimize-complement") {
                minimize_complement = true;
            } else {
                if (input_emp_name.empty()) {
                    input_emp_name = arg;
                } else {
                    throw std::runtime_error("Either some argument is not supported or more than one file was given (try '--help')");
                    exit(-1);
                }
            }
        }

        if (input_emp_name.empty()) {
            throw std::runtime_error("No input given (try --help)");
            exit(-1);
        }

        std::filesystem::path path_to_input(input_emp_name);
        std::filesystem::path path_to_automata = std::filesystem::absolute(path_to_input).parent_path()/"gen_aut";

        std::ifstream input(path_to_input);
        if (!input.is_open()) {
            throw std::runtime_error("could not open file ");
        }

        std::string line;
        std::unordered_map<unsigned, std::istringstream> num_to_operation;
        std::vector<unsigned> input_aut_nums;
        std::vector<Mata::IntermediateAut> input_aut_inter_auts;
        unsigned aut_to_test_emptiness = -1;
        bool test_inclusion = false;
        unsigned aut_to_test_incl1 = -1;
        unsigned aut_to_test_incl2 = -1;
        std::chrono::steady_clock::time_point start;
        std::chrono::steady_clock::time_point end;
        std::chrono::duration<double> elapsed_seconds_parsing;
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
                    throw std::runtime_error(std::string("Could not open file ") + path_to_automaton.string());
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
                start = std::chrono::steady_clock::now();
                input_aut_inter_auts.push_back(Mata::IntermediateAut::parse_from_mf(Mata::Parser::parse_mf(aut_with_split_transitions, true))[0]);
                end = std::chrono::steady_clock::now();
                elapsed_seconds_parsing += end-start;
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

        std::cerr << "parsing: " << elapsed_seconds_parsing.count() << std::endl;
        std::cout << "parsing: " << elapsed_seconds_parsing.count() << std::endl;

        start = std::chrono::steady_clock::now();
        Mata::Mintermization mintermization;
        auto mintermized_input_inter_auts = mintermization.mintermize(input_aut_inter_auts);
        input_aut_inter_auts.clear();
        end = std::chrono::steady_clock::now();
        std::chrono::duration<double> elapsed_seconds = end-start;
        std::cerr << "mintermization: " << elapsed_seconds.count() << std::endl;
        std::cout << "mintermization: " << elapsed_seconds.count() << std::endl;

        std::unordered_map<unsigned, Mata::Nfa::Nfa> num_to_aut;
        Mata::OnTheFlyAlphabet alphabet;
        for (unsigned i = 0; i < mintermized_input_inter_auts.size(); ++i) {
            auto constructed_aut = Mata::Nfa::construct(mintermized_input_inter_auts[i], &alphabet);
            if (REDUCE_SIZE_OF_INPUT) {
                num_to_aut[input_aut_nums[i]] = Mata::Nfa::reduce(constructed_aut);
            } else {
                num_to_aut[input_aut_nums[i]] = constructed_aut;
            }
        }

        std::function<Mata::Nfa::Nfa& (unsigned)> getAutFromNum;
        getAutFromNum = [&num_to_aut, &num_to_operation, &getAutFromNum, &alphabet, REDUCE_SIZE_AFTER_OPERATION, REDUCE_SIZE_OF_RESULT, minimize_complement](unsigned aut_num) -> Mata::Nfa::Nfa& {
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
                if (minimize_complement) {
                    result = Mata::Nfa::complement(getAutFromNum(first_operand_num), alphabet, {{"algorithm", "classical"}, {"minimize", "true"}});
                } else {
                    result = Mata::Nfa::complement(getAutFromNum(first_operand_num), alphabet);
                    if (REDUCE_SIZE_OF_RESULT || REDUCE_SIZE_AFTER_OPERATION) {
                        result = Mata::Nfa::reduce(result);
                    }
                }
            } else {
                result = getAutFromNum(first_operand_num);
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

        if (test_inclusion) {
            if (use_antichain_incl) {
                if (Mata::Nfa::is_included(getAutFromNum(aut_to_test_incl1), getAutFromNum(aut_to_test_incl2))) {
                    std::cout << "result: EMPTY" << std::endl;
                    return 0;
                } else {
                    std::cout << "result: NOT EMPTY" << std::endl;
                    return 0;
                }
            } else {
                Mata::Nfa::Nfa incl_aut = getAutFromNum(aut_to_test_incl2);
                if (minimize_complement) {
                    incl_aut = Mata::Nfa::complement(incl_aut, alphabet, {{"algorithm", "classical"}, {"minimize", "true"}});
                } else {
                    incl_aut = Mata::Nfa::complement(incl_aut, alphabet);
                    if (REDUCE_SIZE_OF_RESULT || REDUCE_SIZE_AFTER_OPERATION) {
                        incl_aut = Mata::Nfa::reduce(incl_aut);
                    }
                }
                incl_aut = Mata::Nfa::intersection(getAutFromNum(aut_to_test_incl1), incl_aut);
                
                if (Mata::Nfa::is_lang_empty(incl_aut)) {
                    std::cout << "result: EMPTY" << std::endl;
                    return 0;
                } else {
                    std::cout << "result: NOT EMPTY" << std::endl;
                    return 0;
                }
            }
        } else {
            if (Mata::Nfa::is_lang_empty(getAutFromNum(aut_to_test_emptiness))) {
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