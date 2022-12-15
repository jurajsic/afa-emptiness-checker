add_executable(afa-emptiness-checker
                    main.cc)

target_link_libraries(afa-emptiness-checker PUBLIC libmata)

# target_include_directories(afa-emptiness-checker PRIVATE ${PROJECT_SOURCE_DIR}/mata/include ${PROJECT_SOURCE_DIR}/mata/3rdparty)
# # target_link_libraries(afa-emptiness-checker PRIVATE mata re2 simlib ${CMAKE_BINARY_DIR}/mata/3rdparty/cudd/lib/libcudd.a)
# target_link_directories(afa-emptiness-checker PRIVATE
#         ${PROJECT_BINARY_DIR}
#         ${PROJECT_BINARY_DIR}/mata/src
#         ${PROJECT_BINARY_DIR}/mata/3rdparty/re2
#         ${PROJECT_BINARY_DIR}/mata/3rdparty/simlib
#         ${PROJECT_BINARY_DIR}/mata/3rdparty/cudd
# )