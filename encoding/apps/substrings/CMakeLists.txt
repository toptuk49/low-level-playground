add_executable(substrings main.c rle_analysis.c)

target_link_libraries(substrings PRIVATE
    arguments
    common
    file
    markov_model
    rle
    m
)

target_include_directories(substrings PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
)
