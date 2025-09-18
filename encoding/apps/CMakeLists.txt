add_subdirectory(file_characteristics)
add_subdirectory(analyze_unicode) 
# add_subdirectory(substrings)

find_program(CLANG_TIDY_EXE NAMES clang-tidy)

if(CLANG_TIDY_EXE)
  set(CMAKE_C_CLANG_TIDY 
    ${CLANG_TIDY_EXE} 
    -extra-arg=-Wno-unknown-warning-option
    --config-file=${CMAKE_SOURCE_DIR}/.clang-tidy
  )
endif()
