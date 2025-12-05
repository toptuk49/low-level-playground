add_library(arithmetic SHARED arithmetic.c)

target_link_libraries(arithmetic PUBLIC 
    common
)

target_include_directories(arithmetic PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
