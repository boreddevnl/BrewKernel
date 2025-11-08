#ifndef FILESYS_H
#define FILESYS_H

#include "file.h"
#include <stdbool.h>
void fs_init(void);
void fs_list_directory(void);
bool fs_change_directory(const char* path);
const char* fs_get_working_directory(void);
bool fs_create_directory(const char* name);
bool fs_list_directory_at_path(const char* path);
void fs_print_working_directory(void);
File* fs_create_file(const char* name);
File* fs_find_file(const char* name);
bool fs_create_directory_at_path(const char* path);
bool fs_remove_file(const char* path);
bool fs_create_directories(const char** names, int count);
const char* fs_read_file_at_path(const char* path, size_t* out_size);

#endif