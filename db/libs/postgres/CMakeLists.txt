add_library(postgres SHARED postgres.c)

target_link_libraries(postgres PUBLIC
    libpq::libpq
)

target_include_directories(postgres PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
)
