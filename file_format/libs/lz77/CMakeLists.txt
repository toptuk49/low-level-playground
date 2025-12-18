add_library(lz77 SHARED lz77.c)

target_link_libraries(lz77 PUBLIC common)

target_include_directories(lz77 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
