add_library(unicode SHARED unicode.c)
target_link_libraries(unicode PUBLIC common)
target_include_directories(unicode PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
