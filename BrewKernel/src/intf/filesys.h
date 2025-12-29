/*
 * Brew Kernel
 * Copyright (C) 2024-2025 boreddevnl
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <https://www.gnu.org/licenses/>.
 */
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
bool fs_write_file_at_path(const char* path, const char* content, size_t size);
bool fs_create_file_at_path(const char* path);

#endif