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
#ifndef MEMORY_H
#define MEMORY_H

#include <stddef.h>

// File content memory pool functions
void* fs_allocate(size_t size);
void fs_free(void* ptr);
size_t fs_get_total_memory(void);
size_t fs_get_used_memory(void);
size_t fs_get_free_memory(void);

// System memory functions
void sys_memory_init(void* multiboot_info);
size_t sys_get_total_ram(void);
size_t sys_get_used_ram(void);
size_t sys_get_free_ram(void);

#endif