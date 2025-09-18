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
static int buffer_pos = 0;
static int in_cli_mode = 0;

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
        brew_str("\nAvailable commands:\n");
        brew_str("  HELP    - Display this help message\n");
        brew_str("  EXIT    - Exit CLI mode and return to regular typing\n");
        brew_str("  CLEAR   - Clear the screen\n");
        brew_str("  ABOUT   - Display system information\n");
        brew_str("  MATH    - Perform basic arithmetic\n");
    }
    else if (strcmp_kernel(cmd_upper, "MATH") == 0) {
        brew_str("\nMath Calculator\n");
        brew_str("Choose operation:\n");
        brew_str("1. Addition (+)\n");
        brew_str("2. Subtraction (-)\n");
        brew_str("3. Multiplication (*)\n");
        brew_str("4. Division (/)\n");
        brew_str("\nEnter operation number: ");
        
        // Clear buffer for new input
        buffer_pos = 0;
        
        // Wait for operation choice
        while (1) {
            if (check_keyboard()) {
                unsigned char scan_code = read_scan_code();
                char ascii_char = scan_code_to_ascii(scan_code);
                
                if (ascii_char >= '1' && ascii_char <= '4') {
                    print_char(ascii_char);
                    char operation = ascii_char;
                    brew_str("\nEnter first number: ");
                    
                    // Get first number
                    buffer_pos = 0;
                    int first_num = 0;
                    while (1) {
                        if (check_keyboard()) {
                            scan_code = read_scan_code();
                            ascii_char = scan_code_to_ascii(scan_code);
                            
                            if (ascii_char >= '0' && ascii_char <= '9') {
                                print_char(ascii_char);
                                first_num = first_num * 10 + (ascii_char - '0');
                            } else if (ascii_char == '\n' || ascii_char == '\r') {
                                break;
                            }
                        }
                    }
                    
                    brew_str("\nEnter second number: ");
                    
                    // Get second number
                    buffer_pos = 0;
                    int second_num = 0;
                    while (1) {
                        if (check_keyboard()) {
                            scan_code = read_scan_code();
                            ascii_char = scan_code_to_ascii(scan_code);
                            
                            if (ascii_char >= '0' && ascii_char <= '9') {
                                print_char(ascii_char);
                                second_num = second_num * 10 + (ascii_char - '0');
                            } else if (ascii_char == '\n' || ascii_char == '\r') {
                                break;
                            }
                        }
                    }
                    
                    brew_str("\nResult: ");
                    switch (operation) {
                        case '1':
                            brew_str("\n");
                            brew_int(first_num);
                            brew_str(" + ");
                            brew_int(second_num);
                            brew_str(" = ");
                            brew_int(first_num + second_num);
                            break;
                        case '2':
                            brew_str("\n");
                            brew_int(first_num);
                            brew_str(" - ");
                            brew_int(second_num);
                            brew_str(" = ");
                            brew_int(first_num - second_num);
                            break;
                        case '3':
                            brew_str("\n");
                            brew_int(first_num);
                            brew_str(" * ");
                            brew_int(second_num);
                            brew_str(" = ");
                            brew_int(first_num * second_num);
                            break;
                        case '4':
                            if (second_num == 0) {
                                brew_str("Error: Division by zero!\n");
                            } else {
                                brew_str("\n");
                                brew_int(first_num);
                                brew_str(" / ");
                                brew_int(second_num);
                                brew_str(" = ");
                                brew_int(first_num / second_num);
                            }
                            break;
                    }
                    brew_str("\n");
                    break;
                }
            }
        }
    }
    else if (strcmp_kernel(cmd_upper, "ABOUT") == 0) {
        brew_str("\nSystem Information:\n");
        print_set_color(PRINT_INDEX_1, PRINT_INDEX_0);
        brew_str("( (\n");
        print_set_color(PRINT_INDEX_2, PRINT_INDEX_0);  
        brew_str("    ) )\n");
        print_set_color(PRINT_INDEX_3, PRINT_INDEX_0);  
        brew_str("  ........\n");
        print_set_color(PRINT_INDEX_4, PRINT_INDEX_0); 
        brew_str("  |      |]\n");
        print_set_color(PRINT_INDEX_5, PRINT_INDEX_0); 
        brew_str("  \\      /\n");
        print_set_color(PRINT_INDEX_9, PRINT_INDEX_0); 
        brew_str("   `----'\n\n");
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);

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
        brew_str("  \\      /\n");
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

static inline void brewing(int iterations) {
    for (volatile int i = 0; i < iterations; i++) {
        __asm__ __volatile__("nop");
    }
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
        brew_str("  \\      /\n");
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
