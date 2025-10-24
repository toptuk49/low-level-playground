add_executable(db main.c)
add_executable(sql_injection sql_injection.c)

target_link_libraries(db PRIVATE
    postgres
)
target_link_libraries(sql_injection PRIVATE
    postgres
)

target_include_directories(db PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(sql_injection PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
