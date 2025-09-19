add_executable(substrings main.c markov_model.c)

target_link_libraries(substrings PRIVATE
    arguments file common m
)

target_include_directories(substrings PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
