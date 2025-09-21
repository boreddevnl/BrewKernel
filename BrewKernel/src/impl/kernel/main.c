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

// Main kernel file for Brew kernel.
// This file contains the main function and the kernel's entry point.
// It prints a welcome message, does a math preview and then loops all VGA characters to the screen.

#include "print.h"
#include "keyboard.h"
#include "rtc.h"
#include "timezones.h"
// Applications (Extensions) includes
#include "APPS/txtedit.h"
#include "APPS/date.h"
#include "APPS/help.h"
#include "APPS/math.h"
#include "APPS/about.h"
#include "APPS/man.h"
#include "APPS/license.h"
#include "APPS/uptime.h"
#include "APPS/doom.h"
#include "APPS/blind.h"
#include "APPS/readtheman.h"
#include "APPS/beep.h"

// String comparison function for kernel
static int strcmp_kernel(const char *s1, const char *s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// CLI state and buffer
static char command_buffer[256];
int buffer_pos = 0;

// Command history
#define HISTORY_SIZE 10
static char command_history[HISTORY_SIZE][256];
static int history_count = 0;      // Number of commands in history
static int history_current = -1;    // Current position in history while navigating
static int history_newest = -1;     // Index of the most recent command

static int in_cli_mode = 0;
static int timezone_offset_h = 0;
static int timezone_offset_m = 0;

// Simple delay function
void brewing(int iterations) {
    for (volatile int i = 0; i < iterations; i++) {
        __asm__ __volatile__("nop");
    }
}
void clistart(){
    brew_str("BrewKernel CLI v1.1\nType HELP for a list of available commands.\n");
}


// Function to store command in history
static void store_command_in_history(const char *cmd) {
    if (cmd[0] == '\0') return;  // Don't store empty commands
    
    // Move to next position in circular buffer
    history_newest = (history_newest + 1) % HISTORY_SIZE;
    
    // Copy command to history
    int i;
    for (i = 0; cmd[i] && i < 255; i++) {
        command_history[history_newest][i] = cmd[i];
    }
    command_history[history_newest][i] = '\0';
    
    // Update history count
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
    
    // Reset navigation
    history_current = -1;
}

// Function to navigate and display command from history
static void navigate_history(int direction) {
    if (history_count == 0) return;
    
    // Save current command if we're just starting to navigate
    if (history_current == -1) {
        command_buffer[buffer_pos] = '\0';
    }
    
    // Update current position
    if (direction > 0) {  // Moving forward (down arrow)
        if (history_current > 0) {
            history_current--;
        } else {
            history_current = -1;  // Return to current command
        }
    } else {  // Moving backward (up arrow)
        if (history_current < history_count - 1) {
            if (history_current == -1) {
                history_current = 0;
            } else {
                history_current++;
            }
        }
    }
    
    // Clear current line
    while (buffer_pos > 0) {
        print_backspace();
        buffer_pos--;
    }
    
    // If we're back at the current command
    if (history_current == -1) {
        return;
    }
    
    // Calculate actual index in circular buffer
    int index = (history_newest - history_current + HISTORY_SIZE) % HISTORY_SIZE;
    
    // Copy and display command from history
    int i;
    for (i = 0; command_history[index][i]; i++) {
        command_buffer[i] = command_history[index][i];
        print_char(command_history[index][i]);
    }
    buffer_pos = i;
}

// Function to process CLI commands
static void process_command(void) {
    // Null terminate the command string
    command_buffer[buffer_pos] = '\0';
    
    // Store command in history
    store_command_in_history(command_buffer);
    
    // Convert command to uppercase for comparison
    char cmd_upper[256];
    int i;
    for(i = 0; command_buffer[i]; i++) {
        cmd_upper[i] = command_buffer[i] >= 'a' && command_buffer[i] <= 'z' 
                     ? command_buffer[i] - 32 
                     : command_buffer[i];
    }
    cmd_upper[i] = '\0';  // Properly null-terminate the string
    
    // Process commands
    if (strcmp_kernel(cmd_upper, "HELP") == 0) {
        display_help();
    }
    else if (strcmp_kernel(cmd_upper, "DATE") == 0) {
        date_command(&timezone_offset_h, &timezone_offset_m);
    }
    else if (strcmp_kernel(cmd_upper, "MATH") == 0) {
        math_cmd();
    }
    else if (strcmp_kernel(cmd_upper, "ABOUT") == 0) {
        display_about();
    }
    else if (strcmp_kernel(cmd_upper, "MAN") == 0) {
        show_manual();
        print_clear();
        clistart();
    }
    else if (strcmp_kernel(cmd_upper, "LICENSE") == 0) {
        show_license();
        print_clear();
        clistart();
    }
    else if (strcmp_kernel(cmd_upper, "UPTIME") == 0) {
        display_uptime();
    }
    else if (strcmp_kernel(cmd_upper, "DOOM") == 0) {
        doom();
    }
    else if (strcmp_kernel(cmd_upper, "BLIND") == 0) {
        blindme();
    }    
    else if (strcmp_kernel(cmd_upper, "CLEAR") == 0) {
        print_clear();
    }
    else if (strcmp_kernel(cmd_upper, "IREADTHEMANUAL") == 0) {
        nerd();
    }
    else if (strcmp_kernel(cmd_upper, "WHOAMI") == 0) {
        brew_str("\n");
        brew_str("idk");
    }  
    else if (strcmp_kernel(cmd_upper, "BEEP") == 0) {
        beep_command();
    }
    else if (strcmp_kernel(cmd_upper, "TXTEDIT") == 0) {
        txtedit_run();
        print_clear();
    }

    else if (strcmp_kernel(cmd_upper, "EXIT") == 0) {
        in_cli_mode = 0;
        print_clear();
        
        // Reprint all kernel initialization output

        // Reprint the logo
        print_set_color(PRINT_INDEX_1, PRINT_INDEX_0);
        brew_str("( (\n");
        print_set_color(PRINT_INDEX_2, PRINT_INDEX_0);  
        brew_str("    ) )\n");
        print_set_color(PRINT_INDEX_3, PRINT_INDEX_0);  
        brew_str("  ........\n");
        print_set_color(PRINT_INDEX_4, PRINT_INDEX_0); 
        brew_str("  |      |]\n");
        print_set_color(PRINT_INDEX_5, PRINT_INDEX_0); 
        brew_str("  \      /\n");
        print_set_color(PRINT_INDEX_9, PRINT_INDEX_0); 
        brew_str("   `----'\n\n");
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);

        // Reprint color palette preview
        for (int i = 0; i < 16; i++) {
            print_set_color(i, i);  
            print_char(' ');       
        }
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("\n\n");

                print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("Brew kernel v2.1\n");
        brew_str("Copyright (C) 2024-2025 boreddevhq.\n");
        brew_str("Build: ");
        brew_str(__DATE__);
        brew_str(" ");
        brew_str(__TIME__);
        brew_str("\n");
        brew_str("Architecture: ");
        #if defined(__x86_64__) || defined(_M_X64)
            brew_str("x86_64");
        #elif defined(__i386__) || defined(_M_IX86)
            brew_str("x86");
        #else
            brew_str("Unknown Architecture");
        #endif
        brew_str("\n");
        brew_str("Compiler: ");
        #if defined(__clang__)
            brew_str("Clang/LLVM ");
            brew_str(__clang_version__);
        #elif defined(__GNUC__)
            brew_str("GCC ");
            brew_str(__VERSION__);
        #else
            brew_str("Unknown Compiler");
        #endif
        brew_str("\n\n");


        // Print CLI instruction
        brew_str("Type 'CLI' and press Enter to start the command line interface...\n");
        brew_str("brew> ");
        buffer_pos = 0;
        return;
    } else if (buffer_pos > 0) {
        brew_str("\nUnknown command. Type HELP for available commands.\n");
    }
    
    // Reset buffer and show prompt
    buffer_pos = 0;
    brew_str("\n> ");
}

// main kernel section,
// do not remove data, only add if needed.
// This is kept for documentation and debugging.
void kernel_main() {
    print_clear();
    
    print_init_palette();

    print_set_palette_color(1, 0, 113, 255);   // Blue
    print_set_palette_color(2, 245, 194, 45);   // Yellow
    print_set_palette_color(3, 255, 129, 63);   // Orange
    print_set_palette_color(4, 237, 28, 36);    // Red
    print_set_palette_color(5, 163, 73, 164);   // Purple
    print_set_palette_color(6, 108, 198, 74);  // Green
    print_set_palette_color(7, 172, 140, 104);  // Latte
    print_set_palette_color(14, 252, 3, 236); // pink

    // Display the Brew kernel logo and system information.
        print_clear();
        
        // Reprint all kernel initialization output

        // Reprint the logo
        print_set_color(PRINT_INDEX_1, PRINT_INDEX_0);
        brew_str("( (\n");
        print_set_color(PRINT_INDEX_2, PRINT_INDEX_0);  
        brew_str("    ) )\n");
        print_set_color(PRINT_INDEX_3, PRINT_INDEX_0);  
        brew_str("  ........\n");
        print_set_color(PRINT_INDEX_4, PRINT_INDEX_0); 
        brew_str("  |      |]\n");
        print_set_color(PRINT_INDEX_5, PRINT_INDEX_0); 
        brew_str("  \      /\n");
        print_set_color(PRINT_INDEX_9, PRINT_INDEX_0); 
        brew_str("   `----'\n\n");
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);

        // Reprint color palette preview
        for (int i = 0; i < 16; i++) {
            print_set_color(i, i);  
            print_char(' ');       
        }
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("\n\n");

                print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("Brew kernel v2.1\n");
        brew_str("Copyright (C) 2024-2025 boreddevhq.\n");
        brew_str("Build: ");
        brew_str(__DATE__);
        brew_str(" ");
        brew_str(__TIME__);
        brew_str("\n");
        brew_str("Architecture: ");
        #if defined(__x86_64__) || defined(_M_X64)
            brew_str("x86_64");
        #elif defined(__i386__) || defined(_M_IX86)
            brew_str("x86");
        #else
            brew_str("Unknown Architecture");
        #endif
        brew_str("\n");
        brew_str("Compiler: ");
        #if defined(__clang__)
            brew_str("Clang/LLVM ");
            brew_str(__clang_version__);
        #elif defined(__GNUC__)
            brew_str("GCC ");
            brew_str(__VERSION__);
        #else
            brew_str("Unknown Compiler");
        #endif
        brew_str("\n\n");


    brew_str("Welcome to Brew kernel!\n");
    brew_str("Type 'CLI' and press Enter to start the command line interface...\n");
    brew_str("brew> ");
    print_enable_cursor();  // Enable the hardware cursor
    
    while (1) {
        if (check_keyboard()) {
            unsigned char scan_code = read_scan_code();
            
            // Handle special keys
            if (scan_code == 0x0E) {  // Backspace
                if (buffer_pos > 0) {
                    buffer_pos--;
                    print_backspace();
                }
            } else if (scan_code == SCAN_CODE_UP_ARROW && in_cli_mode) {
                navigate_history(-1);  // Navigate backwards in history
            } else if (scan_code == SCAN_CODE_DOWN_ARROW && in_cli_mode) {
                navigate_history(1);   // Navigate forwards in history
            } else {
                // Convert scan code to ASCII and print if it's a printable character
                char ascii_char = scan_code_to_ascii(scan_code);
                if (ascii_char != 0) {
                    // Handle special characters
                    if (ascii_char == '\n' || ascii_char == '\r') {
                        if (!in_cli_mode) {
                            // Check if user typed "CLI" to enter CLI mode
                            command_buffer[buffer_pos] = '\0';
                            char cmd_upper[256];
                            int i;
                            for(i = 0; command_buffer[i]; i++) {
                                cmd_upper[i] = command_buffer[i] >= 'a' && command_buffer[i] <= 'z' 
                                           ? command_buffer[i] - 32 
                                           : command_buffer[i];
                            }
                            cmd_upper[i] = '\0';  // Properly null-terminate the string
                            
                            if (strcmp_kernel(cmd_upper, "CLI") == 0) {
                                in_cli_mode = 1;
                                print_clear();
                                clistart();
                                brew_str("brew> ");
                                buffer_pos = 0;
                            } else {
                                brew_str("\n");
                                buffer_pos = 0;
                            }
                        } else {
                            process_command();
                        }
                    } else if (buffer_pos < sizeof(command_buffer) - 1) {
                        // Reset history navigation when typing a new character
                        history_current = -1;
                        command_buffer[buffer_pos++] = ascii_char;
                        print_char(ascii_char);
                    }
                }
            }
            
            brewing(10000000); // Small delay to prevent key repeats
        }
    }
}
