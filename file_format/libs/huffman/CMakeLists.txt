add_library(huffman SHARED entropy.c huffman.c)

target_link_libraries(huffman PUBLIC 
    common file_system
)

target_include_directories(huffman PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
