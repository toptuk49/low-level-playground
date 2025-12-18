add_library(lz78 SHARED lz78.c)

target_link_libraries(lz78 PUBLIC common)

target_include_directories(lz78 PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
