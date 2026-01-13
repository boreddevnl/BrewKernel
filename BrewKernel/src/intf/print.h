/*
 * Brew Kernel
 * Copyright (C) 2024-2026 boreddevnl
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

// Header file for print functions, including:
// -Basic VGA colors
// -RGB color palette
// -Functions to print characters and strings


#pragma once

#include <stdint.h>
#include <stddef.h>

// Color palette structure
typedef struct {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
} VGAColor;

typedef struct {
    VGAColor colors[16];
} ColorPalette;


// Default VGA text colors, changed to custom RGB, but kept for compatiblity and debugging.
enum {
    PRINT_INDEX_0 = 0, // Black
    PRINT_INDEX_1 = 1, // Blue
    PRINT_INDEX_2 = 2, // Green
    PRINT_INDEX_3 = 3, // Cyan
    PRINT_INDEX_4 = 4, // Red
    PRINT_INDEX_5 = 5, // Magenta
    PRINT_INDEX_6 = 6, // Brown
    PRINT_INDEX_7 = 7, // Light grey
    PRINT_INDEX_8 = 8, // Dark grey
    PRINT_INDEX_9 = 9, // Light blue
    PRINT_INDEX_10 = 10, // Light green
    PRINT_INDEX_11 = 11, // Light cyan
    PRINT_INDEX_12 = 12, // Light red
    PRINT_INDEX_13 = 13, // Pink
    PRINT_INDEX_14 = 14, // Yellow
    PRINT_INDEX_15 = 15, // White
};

void print_clear();
void print_char(char character);
void brew_str(const char* string);
void print_set_color(uint8_t foreground, uint8_t background);
void print_init_palette();
void print_set_palette_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue);
void print_load_palette(const ColorPalette* palette);

// Integer printing functions
void brew_int(int number);
void print_uint(unsigned int number);

// Backspace handling
void print_backspace(void);

// Cursor control functions
void print_get_cursor_pos(size_t* row, size_t* col);
void print_set_cursor_pos(size_t row, size_t col);
void print_enable_cursor(void);
void print_disable_cursor(void);
