/*
 * Brew Kernel
 * Copyright (C) 2024-2025 boreddevhq
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 */

#ifndef TXTEDIT_H
#define TXTEDIT_H

#include "../print.h"
#include "../keyboard.h"

#define BUFFER_SIZE 4096
#define MAX_LINES 25
#define MAX_LINE_LENGTH 80
#define MAX_FILES 10
#define MAX_FILENAME 32

// String comparison function for txtedit
static int strcmp_txtedit(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// File structure to store documents in RAM
typedef struct {
    char name[MAX_FILENAME];
    char content[BUFFER_SIZE];
    int size;
} RAMFile;

// Global storage for files
static RAMFile ram_files[MAX_FILES];
static int file_count = 0;
#define ESC_KEY 0x01
#define ENTER_KEY 0x1C
#define BACKSPACE_KEY 0x0E

// RAM storage for the text buffer
static char text_buffer[BUFFER_SIZE];
static int buffer_size = 0;
static int cursor_pos = 0;

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
    brew_str("Enter filename (max 31 chars): ");
    
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
                if(c && pos < MAX_FILENAME - 1) {
                    filename[pos++] = c;
                    print_char(c);
                }
            }
        }
    }
}

static int select_file() {
    int selected = 0;
    int needs_redraw = 1;
    
    while(1) {
        if(needs_redraw) {
            print_clear();
            brew_str("Select a file:\n\n");
            
            // Show NEW FILE option with highlight if selected
            if(selected == 0) {
                print_set_color(PRINT_INDEX_0, PRINT_INDEX_7);
            }
            brew_str("[NEW FILE]\n");
            print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
            
            for(int i = 0; i < file_count; i++) {
                if(i + 1 == selected) {
                    print_set_color(PRINT_INDEX_0, PRINT_INDEX_7);
                }
                brew_str(ram_files[i].name);
                brew_str("\n");
                print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
            }
            needs_redraw = 0;
        }
        
        if(check_keyboard()) {
            unsigned char scan_code = read_scan_code();
            
            if(scan_code == SCAN_CODE_UP_ARROW) {
                if(selected > 0) {
                    selected--;
                    needs_redraw = 1;
                }
            }
            else if(scan_code == SCAN_CODE_DOWN_ARROW) {
                if(selected < file_count) {
                    selected++;
                    needs_redraw = 1;
                }
            }
            else if(scan_code == 0x1C) { // Enter
                return selected - 1; // -1 means new file
            }
        }
    }
}

static void save_current_buffer(const char* text_buffer, int buffer_size) {
    if(file_count >= MAX_FILES) {
        draw_status_line("Error: Maximum number of files reached");
        return;
    }
    
    char filename[MAX_FILENAME];
    get_filename(filename);
    
    // Check if filename already exists
    for(int i = 0; i < file_count; i++) {
        if(strcmp_txtedit(filename, ram_files[i].name) == 0) {
            // Update existing file
            for(int j = 0; j < buffer_size && j < BUFFER_SIZE; j++) {
                ram_files[i].content[j] = text_buffer[j];
            }
            ram_files[i].size = buffer_size;
            draw_status_line("File updated");
            return;
        }
    }
    
    // Create new file
    for(int i = 0; i < MAX_FILENAME; i++) {
        ram_files[file_count].name[i] = filename[i];
        if(filename[i] == '\0') break;
    }
    
    for(int i = 0; i < buffer_size && i < BUFFER_SIZE; i++) {
        ram_files[file_count].content[i] = text_buffer[i];
    }
    ram_files[file_count].size = buffer_size;
    file_count++;
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

void txtedit_run() {
    print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);  // Set initial color to latte
    int file_index = select_file();
    
    // Clear the buffers
    for(int i = 0; i < BUFFER_SIZE; i++) {
        text_buffer[i] = 0;
    }
    buffer_size = 0;
    cursor_pos = 0;
    
    // Load file if selected
    if(file_index >= 0) {
        for(int i = 0; i < ram_files[file_index].size; i++) {
            text_buffer[i] = ram_files[file_index].content[i];
        }
        buffer_size = ram_files[file_index].size;
        cursor_pos = buffer_size;
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
                            save_current_buffer(text_buffer, buffer_size);
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