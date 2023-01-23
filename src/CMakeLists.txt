add_executable(nfa-emptiness-checker
                    main-nfa.cc)

add_executable(afa-emptiness-checker
                    main-afa.cc)

target_link_libraries(nfa-emptiness-checker PUBLIC ${LIBMATA})
target_link_libraries(afa-emptiness-checker PUBLIC ${LIBMATA})
