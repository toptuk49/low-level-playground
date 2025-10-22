#ifndef PATH_UTILS_PATH_UTILS_H
#define PATH_UTILS_PATH_UTILS_H

#include <inttypes.h>
#include <stdbool.h>

#include "types.h"

bool path_utils_is_directory(const char* path);
bool path_utils_exists(const char* path);
Result path_utils_create_directory(const char* path);
char* path_utils_join(const char* dir, const char* file);
char* path_utils_get_parent(const char* path);
const char* path_utils_get_filename(const char* path);
uint64_t path_utils_get_file_size(const char* path);
const char* path_utils_get_relative_path(const char* full_path,
                                         const char* base_path);
bool path_utils_is_file_excluded(const char* filename);

#endif  // PATH_UTILS_PATH_UTILS_H
