add_library(arguments SHARED arguments.c)
target_link_libraries(arguments PUBLIC common)
target_include_directories(arguments PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
