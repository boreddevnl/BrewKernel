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
#include "APPS/date.h"
#include "APPS/help.h"
#include "APPS/math.h"
#include "APPS/about.h"
#include "APPS/man.h"
#include "APPS/license.h"
#include "APPS/uptime.h"

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

static int in_cli_mode = 0;
static int timezone_offset_h = 0;
static int timezone_offset_m = 0;

// Simple delay function
void brewing(int iterations) {
    for (volatile int i = 0; i < iterations; i++) {
        __asm__ __volatile__("nop");
    }
}




// --- License Viewer ---





// Function to process CLI commands
static void process_command(void) {
    // Null terminate the command string
    command_buffer[buffer_pos] = '\0';
    
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
        brew_str("BrewKernel CLI v1.0\nType HELP for a list of available commands.\n");
    }
    else if (strcmp_kernel(cmd_upper, "LICENSE") == 0) {
        show_license();
        print_clear();
        brew_str("BrewKernel CLI v1.0\nType HELP for a list of available commands.\n");
    }
    else if (strcmp_kernel(cmd_upper, "UPTIME") == 0) {
        display_uptime();
    }
    else if (strcmp_kernel(cmd_upper, "CLEAR") == 0) {
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
    print_set_palette_color(1, 108, 198, 74);   // Green
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


    
    brew_str("Type 'CLI' and press Enter to start the command line interface...\n");
    brew_str("> ");
    print_enable_cursor();  // Enable the hardware cursor
    
    while (1) {
        if (check_keyboard()) {
            unsigned char scan_code = read_scan_code();
            
            // Check for backspace key directly (scan code 0x0E)
            if (scan_code == 0x0E) {
                if (buffer_pos > 0) {
                    buffer_pos--;
                    print_backspace();
                }
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
                                brew_str("BrewKernel CLI v1.0\n");
                                brew_str("Type HELP for a list of available commands.\n\n");
                                brew_str("> ");
                                buffer_pos = 0;
                            } else {
                                brew_str("\n");
                                buffer_pos = 0;
                            }
                        } else {
                            process_command();
                        }
                    } else if (buffer_pos < sizeof(command_buffer) - 1) {
                        command_buffer[buffer_pos++] = ascii_char;
                        print_char(ascii_char);
                    }
                }
            }
            
            brewing(10000000); // Small delay to prevent key repeats
        }
    }
}
