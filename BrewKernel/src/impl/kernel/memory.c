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
#include "memory.h"

// Simple memory management for file content

#define MEMORY_SIZE 65536 
static char memory_pool[MEMORY_SIZE];
static size_t memory_used = 0;

void* fs_allocate(size_t size) {
    if (memory_used + size > MEMORY_SIZE) {
        return NULL;  // Out of memory
    }

    void* ptr = &memory_pool[memory_used];
    memory_used += size;
    return ptr;
}

void fs_free(void* ptr) {

}