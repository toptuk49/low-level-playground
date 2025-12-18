add_library(markov_model SHARED markov_model.c)

target_link_libraries(markov_model PUBLIC common)

target_include_directories(markov_model PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
