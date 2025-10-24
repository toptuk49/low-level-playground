find_package(PostgreSQL QUIET)

if(PostgreSQL_FOUND)
    message(STATUS "Found PostgreSQL via FindPostgreSQL")
else()
    find_path(PostgreSQL_INCLUDE_DIR
        NAMES libpq-fe.h
        PATHS /home/linuxbrew/.linuxbrew/opt/libpq/include
              /opt/homebrew/opt/libpq/include
              /usr/local/opt/libpq/include
              /usr/include/postgresql
        NO_DEFAULT_PATH
    )
    
    find_library(PostgreSQL_LIBRARY
        NAMES pq libpq
        PATHS /home/linuxbrew/.linuxbrew/opt/libpq/lib
              /opt/homebrew/opt/libpq/lib
              /usr/local/opt/libpq/lib
              /usr/lib/x86_64-linux-gnu
        NO_DEFAULT_PATH
    )
    
    if(PostgreSQL_INCLUDE_DIR AND PostgreSQL_LIBRARY)
        message(STATUS "Found PostgreSQL manually: ${PostgreSQL_INCLUDE_DIR}, ${PostgreSQL_LIBRARY}")
        set(PostgreSQL_FOUND TRUE)
    else()
        message(FATAL_ERROR "PostgreSQL not found!")
    endif()
endif()

add_library(postgres SHARED postgres.c)

if(PostgreSQL_FOUND)
    target_include_directories(postgres PRIVATE ${PostgreSQL_INCLUDE_DIR})
    target_link_libraries(postgres PRIVATE ${PostgreSQL_LIBRARY})
    target_compile_definitions(postgres PRIVATE HAVE_LIBPQ=1)
endif()

target_include_directories(postgres PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
