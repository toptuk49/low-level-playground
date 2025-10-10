add_executable(db main.c)

target_link_libraries(db PRIVATE
    postgres
)

target_include_directories(db PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
