add_executable(huffman_analyzer main.c analyzer.c)

target_link_libraries(huffman_analyzer PRIVATE
    arguments common huffman file_system
)

target_include_directories(huffman_analyzer PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
