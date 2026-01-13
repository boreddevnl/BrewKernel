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

/* 
 * VGA Text Mode
 * 
 * This file implements basic text output functionality using the VGA text mode buffer.
 * It provides functions for character output, color manipulation, and screen control.
 * 
 * The VGA text mode:
 * - Uses a memory-mapped buffer at 0xB8000
 * - Provides an 80x25 character display
 * - Each character cell consists of:
 *   - 8-bit ASCII character
 *   - 8-bit color attribute (4 bits foreground, 4 bits background)
 * - Supports 16 colors with customizable RGB values
 */

#include "print.h"

// Screen dimensions for standard VGA text mode
const static size_t NUM_COLS = 80;
const static size_t NUM_ROWS = 25;

// Structure representing a character cell in VGA memory
// Each cell contains both the character and its color attributes
struct Char {
    uint8_t character;    // ASCII character
    uint8_t color;       // Color attributes (4 bits FG, 4 bits BG)
};

// VGA text buffer location (memory-mapped I/O)
struct Char* buffer = (struct Char*) 0xb8000;

// Current cursor position
size_t col = 0;
size_t row = 0;

// Current color attribute (white text on black background by default)
uint8_t color = PRINT_INDEX_15 | PRINT_INDEX_0 << 4;

// Default VGA color palette with standard 16-color RGB values
// Each color is stored as 6-bit RGB (0-63 range after conversion)
static ColorPalette default_palette = {
    .colors = {
        {0, 0, 0},       // BLACK (0)
        {0, 0, 170},     // BLUE (1)
        {0, 170, 0},     // GREEN (2)
        {0, 170, 170},   // CYAN (3)
        {170, 0, 0},     // RED (4)
        {170, 0, 170},   // MAGENTA (5)
        {170, 85, 0},    // BROWN (6)
        {170, 170, 170}, // LIGHT_GRAY (7)
        {85, 85, 85},    // DARK_GRAY (8)
        {85, 85, 255},   // LIGHT_BLUE (9)
        {85, 255, 85},   // LIGHT_GREEN (10)
        {85, 255, 255},  // LIGHT_CYAN (11)
        {255, 85, 85},   // LIGHT_RED (12)
        {255, 85, 255},  // PINK (13)
        {255, 255, 85},  // YELLOW (14)
        {255, 255, 255}  // WHITE (15)
    }
};

/*
 * Port I/O Helper Functions
 */

// Write a byte to an I/O port
static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

// Read a byte from an I/O port
static inline uint8_t inb(uint16_t port) {
    uint8_t value;
    asm volatile("inb %1, %0" : "=a"(value) : "Nd"(port));
    return value;
}

/*
 * Color Palette Management Functions
 */

// Set an individual color in the VGA DAC (Digital-to-Analog Converter)
void print_set_palette_color(uint8_t index, uint8_t red, uint8_t green, uint8_t blue) {
    // Wait for vertical retrace to prevent screen flickering
    do {
        inb(0x3DA);
    } while (inb(0x3DA) & 8);

    // Program the DAC registers
    outb(0x3C8, index);          // Select palette index
    outb(0x3C9, red >> 2);       // Convert 8-bit RGB to 6-bit DAC values
    outb(0x3C9, green >> 2);
    outb(0x3C9, blue >> 2);
}

// Load an entire 16-color palette into the VGA DAC
void print_load_palette(const ColorPalette* palette) {
    for (int i = 0; i < 16; i++) {
        print_set_palette_color(i, 
            palette->colors[i].red,
            palette->colors[i].green,
            palette->colors[i].blue);
    }
}

// Initialize VGA with the default color palette
void print_init_palette() {
    print_load_palette(&default_palette);
}

/*
 * Screen Management Functions
 */

// Clear a single row by filling it with spaces
void clear_row(size_t row) {
    struct Char empty = (struct Char) {
        character: ' ',
        color: color,
    };

    for (size_t col = 0; col < NUM_COLS; col++) {
        buffer[col + NUM_COLS * row] = empty;
    }
}

// Clear the entire screen
void print_clear() {
    for (size_t i = 0; i < NUM_ROWS; i++) {
        clear_row(i);
    }
    // Reset cursor to top-left corner
    row = 0;
    col = 0;
    print_set_cursor_pos(0, 0);
}

// Handle newline by moving cursor to start of next line
// If at bottom of screen, scroll everything up one line
void print_newline() {
    col = 0;

    if (row < NUM_ROWS - 1) {
        row++;
        print_set_cursor_pos(row, col);  // Update hardware cursor position
        return;
    }

    // Scroll the screen up one line
    for (size_t row = 1; row < NUM_ROWS; row++) {
        for (size_t col = 0; col < NUM_COLS; col++) {
            struct Char character = buffer[col + NUM_COLS * row];
            buffer[col + NUM_COLS * (row - 1)] = character;
        }
    }
    clear_row(NUM_ROWS - 1);
}

/*
 * Text Output Functions
 */

// Print a single character at the current cursor position
// Handles special characters like newline and implements line wrapping
void print_char(char character) {
    if (character == '\n') {
        print_newline();
        return;
    }

    if (col >= NUM_COLS) {
        print_newline();
    }

    buffer[col + NUM_COLS * row] = (struct Char) {
        character: (uint8_t) character,
        color: color,
    };

    col++;
    print_set_cursor_pos(row, col);  // Update hardware cursor position
}

// Print a null-terminated string
void brew_str(const char* str) {
    for (size_t i = 0; 1; i++) {
        char character = (uint8_t) str[i];

        if (character == '\0') {
            return;
        }

        print_char(character);
    }
}

// Set the current text color attributes
// foreground and background are indices into the color palette (0-15)
void print_set_color(uint8_t foreground, uint8_t background) {
    color = foreground + (background << 4);
}

// Convert unsigned integer to string and print it
void print_uint(unsigned int number) {
    char buffer[32];
    int pos = 0;
    
    // Handle 0 case
    if (number == 0) {
        print_char('0');
        return;
    }
    
    // Convert digits to characters from right to left
    while (number > 0) {
        buffer[pos++] = '0' + (number % 10);
        number = number / 10;
    }
    
    // Print digits in reverse order
    while (pos > 0) {
        print_char(buffer[--pos]);
    }
}

// Convert signed integer to string and print it
void brew_int(int number) {
    if (number < 0) {
        print_char('-');
        print_uint(-number);
    } else {
        print_uint(number);
    }
}

// Get the current cursor position
void print_get_cursor_pos(size_t* cur_row, size_t* cur_col) {
    *cur_row = row;
    *cur_col = col;
}

// Set the cursor position
void print_set_cursor_pos(size_t new_row, size_t new_col) {
    if (new_row >= NUM_ROWS) new_row = NUM_ROWS - 1;
    if (new_col >= NUM_COLS) new_col = NUM_COLS - 1;
    
    row = new_row;
    col = new_col;
    
    // Calculate linear offset
    uint16_t pos = row * NUM_COLS + col;
    
    // Update hardware cursor
    outb(0x3D4, 0x0F);
    outb(0x3D5, (uint8_t)(pos & 0xFF));
    outb(0x3D4, 0x0E);
    outb(0x3D5, (uint8_t)((pos >> 8) & 0xFF));
}

// Enable the hardware cursor
void print_enable_cursor(void) {
    // Get current cursor shape
    outb(0x3D4, 0x0A);
    uint8_t cursor_start = inb(0x3D5) & 0xC0;  // Preserve top bits
    
    // Set cursor start line (usually 13)
    outb(0x3D4, 0x0A);
    outb(0x3D5, cursor_start | 13);
    
    // Set cursor end line (usually 14)
    outb(0x3D4, 0x0B);
    outb(0x3D5, 14);
}

// Disable the hardware cursor
void print_disable_cursor(void) {
    outb(0x3D4, 0x0A);
    outb(0x3D5, 0x20);
}

// Handle backspace by moving cursor back and clearing the character
void print_backspace(void) {
    if (col > 0) {
        // Move back one column on current line
        col--;
    } else if (row > 0) {
        // Move to end of previous line
        row--;
        col = NUM_COLS - 1;
        
        // Find the last non-space character on the previous line
        while (col > 0 && buffer[col + NUM_COLS * row].character == ' ') {
            col--;
        }
        // Move one past the last character
        if (buffer[col + NUM_COLS * row].character != ' ') {
            col++;
        }
    } else {
        // At beginning of first line, nothing to do
        return;
    }

    // Clear the character at current position
    buffer[col + NUM_COLS * row] = (struct Char) {
        character: ' ',
        color: color,
    };

    // Update hardware cursor position
    print_set_cursor_pos(row, col);
}
