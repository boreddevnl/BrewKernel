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
#ifndef APPS_MEMORY_H
#define APPS_MEMORY_H

#include "../print.h"
#include "../memory.h"

// Function to print a number with commas (simplified - just print the number)
static void display_memory(void) {
    // File content pool memory
    size_t file_total = fs_get_total_memory();
    size_t file_used = fs_get_used_memory();
    size_t file_free = fs_get_free_memory();
    
    // System RAM
    size_t sys_total = sys_get_total_ram();
    size_t sys_used = sys_get_used_ram();
    size_t sys_free = sys_get_free_ram();
    
    // Calculate percentages
    unsigned int file_percent_used = 0;
    if (file_total > 0) {
        file_percent_used = (unsigned int)((file_used * 100) / file_total);
    }
    
    unsigned int sys_percent_used = 0;
    if (sys_total > 0) {
        sys_percent_used = (unsigned int)((sys_used * 100) / sys_total);
    }
    
    brew_str("\n=== File Content Pool ===\n");
    brew_str("  Total: ");
    brew_int((int)file_total);
    brew_str(" bytes (");
    brew_int((int)(file_total / 1024));
    brew_str(" KB)\n");
    
    brew_str("  Used:  ");
    brew_int((int)file_used);
    brew_str(" bytes (");
    brew_int((int)(file_used / 1024));
    brew_str(" KB) - ");
    brew_int(file_percent_used);
    brew_str("%\n");
    
    brew_str("  Free:  ");
    brew_int((int)file_free);
    brew_str(" bytes (");
    brew_int((int)(file_free / 1024));
    brew_str(" KB) - ");
    brew_int(100 - file_percent_used);
    brew_str("%\n");
    
    brew_str("\n=== System RAM ===\n");
    brew_str("  Total: ");
    brew_int((int)(sys_total / 1024 / 1024));
    brew_str(" MB (");
    brew_int((int)sys_total);
    brew_str(" bytes)\n");
    
    brew_str("  Used:  ");
    brew_int((int)(sys_used / 1024 / 1024));
    brew_str(" MB (");
    brew_int((int)sys_used);
    brew_str(" bytes) - ");
    brew_int(sys_percent_used);
    brew_str("%\n");
    
    brew_str("  Free:  ");
    brew_int((int)(sys_free / 1024 / 1024));
    brew_str(" MB (");
    brew_int((int)sys_free);
    brew_str(" bytes) - ");
    brew_int(100 - sys_percent_used);
    brew_str("%\n");
}

#endif // APPS_MEMORY_H

