add_executable(nfa-emptiness-checker
                    main-nfa.cc)

target_link_libraries(nfa-emptiness-checker PUBLIC ${LIBMATA} libvata)
