/*
 * Brew Kernel
 * Copyright (C) 2024-2025 boreddevhq
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
#ifndef FILE_H
#define FILE_H

#include <stddef.h>
#include <stdbool.h>

#define FS_MAX_FILENAME 256
#define FS_MAX_FILES 100
#define FS_MAX_FILE_SIZE 4096

typedef struct File {
    char name[FS_MAX_FILENAME];
    char type;              // 'd' for directory, 'f' for file
    struct File* parent;    // Parent directory
    size_t child_count;     // Number of children (for directories)
    struct File* children;  // First child in linked list
    struct File* next_sibling; // Next sibling in parent's children list
    char* content;          // File content (NULL for directories)
    size_t content_size;    // Size of content (0 for directories)
} File;

File* create_file(const char* name, char type);
void cleanup_filesystem(void);
bool file_write_content(File* file, const char* content, size_t size);
const char* file_get_content(const File* file, size_t* size);

#endif