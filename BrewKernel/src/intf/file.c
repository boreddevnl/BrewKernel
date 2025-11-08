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
#include "file.h"
#include "print.h"
#include "memory.h"
#include <stddef.h>

static void fs_memcpy(void* dest, const void* src, size_t n) {
    char* d = dest;
    const char* s = src;
    while (n--) *d++ = *s++;
}

File* create_file(const char* name, char type) {
    static File files[FS_MAX_FILES];
    static size_t file_count = 0;
    
    if (file_count >= FS_MAX_FILES) {
        return NULL;
    }

    File* file = &files[file_count++];
    
    size_t i;
    for (i = 0; name[i] && i < FS_MAX_FILENAME - 1; i++) {
        file->name[i] = name[i];
    }
    file->name[i] = '\0';
    
    file->type = type;
    file->parent = NULL;
    file->child_count = 0;
    file->children = NULL;
    file->next_sibling = NULL;
    file->content = NULL;
    file->content_size = 0;
    
    return file;
}

bool file_write_content(File* file, const char* content, size_t size) {
    if (!file || file->type != 'f' || size > FS_MAX_FILE_SIZE) {
        return false;
    }

    if (file->content) {
        fs_free(file->content);
        file->content = NULL;
        file->content_size = 0;
    }

    char* new_content = fs_allocate(size);
    if (!new_content) {
        return false;
    }

    fs_memcpy(new_content, content, size);
    file->content = new_content;
    file->content_size = size;

    return true;
}

const char* file_get_content(const File* file, size_t* size) {
    if (!file || file->type != 'f' || !size) {
        return NULL;
    }

    *size = file->content_size;
    return file->content;
}

void cleanup_filesystem(void) {

}