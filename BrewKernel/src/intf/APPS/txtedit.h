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

#ifndef TXTEDIT_H
#define TXTEDIT_H

#include "../print.h"
#include "../keyboard.h"
#include "../filesys.h"
#include "../file.h"

#define BUFFER_SIZE 4096
#define MAX_LINES 25
#define MAX_LINE_LENGTH 80

// Helper string functions for txtedit
static int strcmp_txtedit(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

static size_t strlen_txtedit(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

// Buffer for the current file being edited
static char text_buffer[BUFFER_SIZE];
static int buffer_size = 0;
static int cursor_pos = 0;
#define ESC_KEY 0x01
#define ENTER_KEY 0x1C
#define BACKSPACE_KEY 0x0E

// Status messages
static const char* MSG_SAVED = "Text saved to RAM buffer";
static const char* MSG_LOADED = "Text loaded from RAM buffer";
static const char* MSG_HELP = "ESC:Exit ENTER:NewLine";

static void draw_status_line(const char* msg) {
    size_t row, col;
    print_get_cursor_pos(&row, &col);
    print_set_cursor_pos(24, 0);  // Bottom line
    print_set_color(PRINT_INDEX_0, PRINT_INDEX_7);  // Black on light grey
    for(int i = 0; i < 80; i++) print_char(' ');  // Clear line
    print_set_cursor_pos(24, 0);
    brew_str(msg);
    print_set_cursor_pos(row, col);
    print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);  // Reset colors to latte
}

static void get_filename(char* filename) {
    int pos = 0;
    print_clear();
    brew_str("Enter filename (e.g., file.txt): ");
    
    while(1) {
        if(check_keyboard()) {
            unsigned char scan_code = read_scan_code();
            
            if(scan_code == 0x1C) { // Enter
                filename[pos] = '\0';
                break;
            }
            else if(scan_code == 0x0E) { // Backspace
                if(pos > 0) {
                    pos--;
                    print_backspace();
                }
            }
            else {
                char c = scan_code_to_ascii(scan_code);
                if(c && pos < FS_MAX_FILENAME - 1) {
                    filename[pos++] = c;
                    print_char(c);
                }
            }
        }
    }
}

static char current_filename[FS_MAX_FILENAME];

static void save_current_buffer(const char* text_buffer, int buffer_size, bool prompt_for_filename) {
    // If we don't have a current filename and need to prompt, ask for one
    if (current_filename[0] == '\0' && prompt_for_filename) {
        get_filename(current_filename);
    } else if (current_filename[0] == '\0') {
        draw_status_line("Error: No filename specified");
        return;
    }
    
    // Create a new file in the current directory
    File* file = fs_find_file(current_filename);
    if (!file) {
        file = fs_create_file(current_filename);
    }
    
    if (file && file_write_content(file, text_buffer, buffer_size)) {
        draw_status_line("File saved successfully");
    } else {
        draw_status_line("Error: Could not save file");
    }
}

static void redraw_screen() {
    print_clear();
    print_set_cursor_pos(0, 0);
    
    // Draw the text
    for(int i = 0; i < buffer_size; i++) {
        print_char(text_buffer[i]);
    }
    
    draw_status_line(MSG_HELP);
}

static void insert_char(char c) {
    if(buffer_size >= BUFFER_SIZE - 1) return;
    
    // Move following text forward
    for(int i = buffer_size; i > cursor_pos; i--) {
        text_buffer[i] = text_buffer[i-1];
    }
    
    text_buffer[cursor_pos] = c;
    cursor_pos++;
    buffer_size++;
    print_char(c);
}

static void delete_char() {
    if(cursor_pos <= 0) return;
    
    cursor_pos--;
    // Move following text backward
    for(int i = cursor_pos; i < buffer_size - 1; i++) {
        text_buffer[i] = text_buffer[i+1];
    }
    buffer_size--;
    
    print_backspace();
}

static void handle_special_key(unsigned char scan_code) {
    size_t row, col;
    print_get_cursor_pos(&row, &col);
    
    switch(scan_code) {
        case SCAN_CODE_UP_ARROW:
            if(row > 0) {
                print_set_cursor_pos(row-1, col);
                cursor_pos -= MAX_LINE_LENGTH;
                if(cursor_pos < 0) cursor_pos = 0;
            }
            break;
            
        case SCAN_CODE_DOWN_ARROW:
            if(row < MAX_LINES-2 && cursor_pos + MAX_LINE_LENGTH <= buffer_size) {
                print_set_cursor_pos(row+1, col);
                cursor_pos += MAX_LINE_LENGTH;
                if(cursor_pos > buffer_size) cursor_pos = buffer_size;
            }
            break;
    }
}

static void load_file(const char* filename) {
    File* file = fs_find_file(filename);
    if (!file) {
        draw_status_line("Creating new file");
        return;
    }

    size_t size;
    const char* content = file_get_content(file, &size);
    if (!content || size > BUFFER_SIZE) {
        draw_status_line("Error: Could not load file or file too large");
        return;
    }

    // Copy content to buffer
    for (size_t i = 0; i < size; i++) {
        text_buffer[i] = content[i];
    }
    buffer_size = size;
    cursor_pos = size;
    draw_status_line("File loaded successfully");
}

void txtedit_run(const char* filename) {
    print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);  // Set initial color to latte
    
    // Clear the buffers and filename
    for(int i = 0; i < BUFFER_SIZE; i++) {
        text_buffer[i] = 0;
    }
    buffer_size = 0;
    cursor_pos = 0;
    current_filename[0] = '\0';

    // If filename is provided, store it
    if (filename) {
        int i;
        for (i = 0; filename[i] && i < FS_MAX_FILENAME - 1; i++) {
            current_filename[i] = filename[i];
        }
        current_filename[i] = '\0';
        load_file(filename);
    }
    
    print_clear();
    redraw_screen();
    print_enable_cursor();
    
    while(1) {
        if(check_keyboard()) {
            unsigned char scan_code = read_scan_code();
            
            if(scan_code == ESC_KEY) {
                // Ask to save before exit
                draw_status_line("Save before exit? (Y/N)");
                while(1) {
                    if(check_keyboard()) {
                        unsigned char save_scan = read_scan_code();
                        char save_char = scan_code_to_ascii(save_scan);
                        if(save_char == 'Y' || save_char == 'y') {
                            // Only prompt for filename if we don't have one (started with txtedit with no args)
                            save_current_buffer(text_buffer, buffer_size, current_filename[0] == '\0');
                            break;
                        }
                        else if(save_char == 'N' || save_char == 'n') {
                            break;
                        }
                    }
                }
                break;
            }
            else if(scan_code == ENTER_KEY) {
                insert_char('\n');
            }
            else if(scan_code == BACKSPACE_KEY) {
                delete_char();
            }
            else if(is_special_key(scan_code)) {
                handle_special_key(scan_code);
            }
            else {
                char c = scan_code_to_ascii(scan_code);
                if(c) insert_char(c);
            }
        }
    }
    

}

#endif // TXTEDIT_H