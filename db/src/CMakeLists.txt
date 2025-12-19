add_executable(auth_crud auth_crud.c)
add_executable(crud_demonstration crud_demonstration.c)
add_executable(db main.c)
add_executable(query_executor query_executor.c)
add_executable(representations representations.c)
add_executable(sql_injection sql_injection.c)

target_link_libraries(auth_crud PRIVATE
    postgres
)

target_link_libraries(crud_demonstration PRIVATE
    postgres
)

target_link_libraries(db PRIVATE
    postgres
)

target_link_libraries(query_executor PRIVATE
    postgres
)

target_link_libraries(representations PRIVATE
    postgres
)

target_link_libraries(sql_injection PRIVATE
    postgres
)

target_include_directories(auth_crud PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(crud_demonstration PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(db PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(query_executor PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(representations PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)

target_include_directories(sql_injection PRIVATE 
    ${CMAKE_CURRENT_SOURCE_DIR}
)
