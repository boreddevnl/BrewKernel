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
#include <stdint.h>

// Memory management for file content using a free list allocator

#define MEMORY_SIZE 1048576  // 1MB - file storage capacity (reasonable size for static array)
#define ALIGNMENT 8  // Align blocks to 8 bytes
#define MIN_BLOCK_SIZE (sizeof(BlockHeader) + ALIGNMENT)

// Block header structure
typedef struct BlockHeader {
    size_t size;           // Size of the block (excluding header)
    int is_free;           // 1 if free, 0 if allocated
    struct BlockHeader* next;  // Next block in free list
} BlockHeader;

static char memory_pool[MEMORY_SIZE];
static BlockHeader* free_list = NULL;
static int memory_initialized = 0;

// Align size to ALIGNMENT boundary
static size_t align_size(size_t size) {
    return (size + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// Initialize memory pool
static void init_memory(void) {
    if (memory_initialized) return;
    
    // Initialize the entire pool as one large free block
    BlockHeader* header = (BlockHeader*)memory_pool;
    header->size = MEMORY_SIZE - sizeof(BlockHeader);
    header->is_free = 1;
    header->next = NULL;
    free_list = header;
    memory_initialized = 1;
}

// Split a free block if it's large enough
static void split_block(BlockHeader* block, size_t size) {
    size_t remaining = block->size - size;
    if (remaining >= MIN_BLOCK_SIZE) {
        // Create new free block from remaining space
        BlockHeader* new_block = (BlockHeader*)((char*)block + sizeof(BlockHeader) + size);
        new_block->size = remaining - sizeof(BlockHeader);
        new_block->is_free = 1;
        new_block->next = free_list;  // Add to free list
        
        block->size = size;
        free_list = new_block;  // Add new block to free list
    }
}

// Coalesce adjacent free blocks
static void coalesce_blocks(void) {
    BlockHeader* current = (BlockHeader*)memory_pool;
    BlockHeader* end = (BlockHeader*)(memory_pool + MEMORY_SIZE);
    
    while ((char*)current < (char*)end) {
        if (current->is_free) {
            // Check if next block is adjacent and free
            BlockHeader* next = (BlockHeader*)((char*)current + sizeof(BlockHeader) + current->size);
            if ((char*)next < (char*)end && next->is_free) {
                // Merge blocks
                current->size += sizeof(BlockHeader) + next->size;
                // Update free list
                BlockHeader* prev = NULL;
                BlockHeader* walk = free_list;
                while (walk) {
                    if (walk == next) {
                        if (prev) {
                            prev->next = walk->next;
                        } else {
                            free_list = walk->next;
                        }
                        break;
                    }
                    prev = walk;
                    walk = walk->next;
                }
                // Continue checking from current position
                continue;
            }
        }
        // Move to next block
        current = (BlockHeader*)((char*)current + sizeof(BlockHeader) + current->size);
    }
}

void* fs_allocate(size_t size) {
    if (size == 0) return NULL;
    
    init_memory();
    
    // Align size
    size = align_size(size);
    if (size < MIN_BLOCK_SIZE - sizeof(BlockHeader)) {
        size = MIN_BLOCK_SIZE - sizeof(BlockHeader);
    }
    
    // Find a suitable free block
    BlockHeader* prev = NULL;
    BlockHeader* current = free_list;
    
    while (current) {
        if (current->is_free && current->size >= size) {
            // Found a suitable block
            current->is_free = 0;
            
            // Remove from free list
            if (prev) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
            
            // Split if large enough
            split_block(current, size);
            
            // Return pointer to data (after header)
            return (char*)current + sizeof(BlockHeader);
        }
        prev = current;
        current = current->next;
    }
    
    // Try coalescing and search again
    coalesce_blocks();
    
    // Search again after coalescing
    prev = NULL;
    current = free_list;
    while (current) {
        if (current->is_free && current->size >= size) {
            current->is_free = 0;
            if (prev) {
                prev->next = current->next;
            } else {
                free_list = current->next;
            }
            split_block(current, size);
            return (char*)current + sizeof(BlockHeader);
        }
        prev = current;
        current = current->next;
    }
    
    return NULL;  // Out of memory
}

void fs_free(void* ptr) {
    if (!ptr) return;
    
    // Get block header
    BlockHeader* header = (BlockHeader*)((char*)ptr - sizeof(BlockHeader));
    
    // Validate pointer is within memory pool
    if ((char*)header < memory_pool || (char*)header >= memory_pool + MEMORY_SIZE) {
        return;  // Invalid pointer
    }
    
    if (header->is_free) {
        return;  // Already free (double free)
    }
    
    // Mark as free
    header->is_free = 1;
    
    // Add to free list
    header->next = free_list;
    free_list = header;
    
    // Coalesce adjacent free blocks
    coalesce_blocks();
}

// Get memory statistics
size_t fs_get_total_memory(void) {
    return MEMORY_SIZE;
}

size_t fs_get_used_memory(void) {
    init_memory();  // Ensure memory is initialized
    
    size_t used = 0;
    BlockHeader* current = (BlockHeader*)memory_pool;
    BlockHeader* end = (BlockHeader*)(memory_pool + MEMORY_SIZE);
    
    while ((char*)current < (char*)end) {
        if (!current->is_free) {
            used += sizeof(BlockHeader) + current->size;
        }
        // Move to next block
        size_t block_size = sizeof(BlockHeader) + current->size;
        current = (BlockHeader*)((char*)current + block_size);
        
        // Safety check to prevent infinite loop
        if (block_size == 0) break;
    }
    
    return used;
}

size_t fs_get_free_memory(void) {
    init_memory();  // Ensure memory is initialized
    return MEMORY_SIZE - fs_get_used_memory();
}

// System memory detection using Multiboot info
static size_t system_total_ram = 0;
static int system_memory_initialized = 0;

// Multiboot info structure (simplified - we only need memory fields)
struct multiboot_info {
    uint32_t flags;
    uint32_t mem_lower;
    uint32_t mem_upper;
    // ... other fields we don't need
};

void sys_memory_init(void* multiboot_info_ptr) {
    if (system_memory_initialized) return;
    
    if (!multiboot_info_ptr) {
        // No multiboot info, assume default (e.g., 512MB)
        system_total_ram = 512 * 1024 * 1024;  // 512MB default
        system_memory_initialized = 1;
        return;
    }
    
    struct multiboot_info* mb_info = (struct multiboot_info*)multiboot_info_ptr;
    
    // Check if memory info is available (bit 0 of flags)
    if (mb_info->flags & 0x01) {
        // mem_lower is in KB (typically 0-640KB)
        // mem_upper is in KB (typically 1MB+)
        // Total RAM = (mem_lower + mem_upper) * 1024 bytes
        uint32_t total_kb = mb_info->mem_lower + mb_info->mem_upper;
        system_total_ram = (size_t)total_kb * 1024;
    } else {
        // Memory info not available, use default
        system_total_ram = 512 * 1024 * 1024;  // 512MB default
    }
    
    system_memory_initialized = 1;
}

size_t sys_get_total_ram(void) {
    if (!system_memory_initialized) {
        // Not initialized yet, return default
        return 512 * 1024 * 1024;  // 512MB default
    }
    return system_total_ram;
}

size_t sys_get_used_ram(void) {
    // Estimate kernel memory usage:
    // - File content pool: fs_get_used_memory()
    // - Kernel code/data/stack: rough estimate
    // - Static arrays and structures
    
    size_t file_pool_used = fs_get_used_memory();
    
    // Rough estimate of kernel overhead:
    // - Kernel code: ~100KB (rough estimate)
    // - Stack: 16KB (from boot code)
    // - Static data: ~50KB (rough estimate)
    // - Page tables: ~12KB (3 * 4KB)
    size_t kernel_overhead = 100 * 1024 + 16 * 1024 + 50 * 1024 + 12 * 1024;  // ~178KB
    
    return file_pool_used + kernel_overhead;
}

size_t sys_get_free_ram(void) {
    size_t total = sys_get_total_ram();
    size_t used = sys_get_used_ram();
    
    if (used > total) return 0;  // Safety check
    return total - used;
}