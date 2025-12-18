add_library(rle SHARED rle.c)

target_link_libraries(rle PUBLIC common)

target_include_directories(rle PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
