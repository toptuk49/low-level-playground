#ifndef FILE_SYSTEM_FILE_SYSTEM_H
#define FILE_SYSTEM_FILE_SYSTEM_H

#include "types.h"

typedef struct FileList FileList;
typedef struct DirectoryTree DirectoryTree;

// Список файлов
FileList* file_list_create(void);
void file_list_destroy(FileList* self);
Result file_list_add_file(FileList* self, const char* filename);
Result file_list_add_directory(FileList* self, const char* dirname,
                               bool recursive);
Size file_list_get_count(const FileList* self);
const char* file_list_get_path(const FileList* self, Size index);
bool file_list_is_directory(const FileList* self, Size index);
uint64_t file_list_get_size(const FileList* self, Size index);

// Дерево директорий
DirectoryTree* directory_tree_create(void);
void directory_tree_destroy(DirectoryTree* self);
Result directory_tree_build_from_path(DirectoryTree* self,
                                      const char* root_path);
Result directory_tree_create_from_archive(DirectoryTree* self,
                                          const char* archive_path);
FileList* directory_tree_get_file_list(DirectoryTree* self);

// Дополнительные утилиты файловой системы
Result file_system_create_directory(const char* path);
bool file_system_path_exists(const char* path);
Result file_system_copy_file(const char* source, const char* destination);

#endif  // FILE_SYSTEM_FILE_SYSTEM_H
