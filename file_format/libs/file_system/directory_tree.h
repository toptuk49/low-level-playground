#ifndef FILE_SYSTEM_DIRECTORY_TREE_H
#define FILE_SYSTEM_DIRECTORY_TREE_H

#include "file_list.h"
#include "types.h"

typedef struct DirectoryTree DirectoryTree;

DirectoryTree* directory_tree_create(void);
void directory_tree_destroy(DirectoryTree* self);

Result directory_tree_build_from_path(DirectoryTree* self,
                                      const char* root_path);
Result directory_tree_create_from_archive(DirectoryTree* self,
                                          const char* archive_path);
FileList* directory_tree_get_file_list(DirectoryTree* self);

#endif  //  FILE_SYSTEM_DIRECTORY_TREE_H
