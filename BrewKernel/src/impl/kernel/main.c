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

// Main kernel file for Brew kernel.
// This file contains the main function and the kernel's entry point.
// it contains the cli (Command line interface) implementation, using header files
// (example APPS/txtedit.h). and then calls functions from those header files
// for use in the CLI.
#include "print.h"
#include "keyboard.h"
#include "rtc.h"
#include "timezones.h"
#include "APPS/txtedit.h"
#include "APPS/date.h"
#include "APPS/help.h"
#include "APPS/math.h"
#include "APPS/about.h"
#include "APPS/man.h"
#include "APPS/license.h"
#include "APPS/uptime.h"
#include "APPS/blind.h"
#include "APPS/readtheman.h"
#include "APPS/beep.h"
#include "APPS/shutdown.h"
#include "APPS/reboot.h"
#include "APPS/cowsay.h"
#include "APPS/brewer.h"
#include "APPS/memory.h"
#include "memory.h"
#include "filesys.h"
#include "pic.h"
#include "irq.h"
#include "timer.h"
#include "pci.h"
#include "network.h"
#include "e1000.h"
#include "network_cli.h"
#include "shell_cli.h"

// External assembly function to initialize IDT
extern void init_idt(void);

// Enable to automatically enter the CLI on boot. Set to 0 to disable this, probably no reason to do this might be handy though.
#ifndef AUTO_START_CLI
#define AUTO_START_CLI 1
#endif

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


void brewing(int iterations) {
    for (volatile int i = 0; i < iterations; i++) {
        __asm__ __volatile__("nop");
    }
}
void clistart(){
    brew_str("BrewKernel CLI v1.2\nType HELP for a list of available commands.\n");
}


static void store_command_in_history(const char *cmd) {
    if (cmd[0] == '\0') return;  
    
    history_newest = (history_newest + 1) % HISTORY_SIZE;
    
    int i;
    for (i = 0; cmd[i] && i < 255; i++) {
        command_history[history_newest][i] = cmd[i];
    }
    command_history[history_newest][i] = '\0';
    
    if (history_count < HISTORY_SIZE) {
        history_count++;
    }
    
    history_current = -1;
}

static void navigate_history(int direction) {
    if (history_count == 0) return;
    
    if (history_current == -1) {
        command_buffer[buffer_pos] = '\0';
    }
    
    if (direction > 0) {  
        if (history_current > 0) {
            history_current--;
        } else {
            history_current = -1;  
        }
    } else {  
        if (history_current < history_count - 1) {
            if (history_current == -1) {
                history_current = 0;
            } else {
                history_current++;
            }
        }
    }
    
    while (buffer_pos > 0) {
        print_backspace();
        buffer_pos--;
    }
    
    if (history_current == -1) {
        return;
    }
    
    int index = (history_newest - history_current + HISTORY_SIZE) % HISTORY_SIZE;
    
    int i;
    for (i = 0; command_history[index][i]; i++) {
        command_buffer[i] = command_history[index][i];
        print_char(command_history[index][i]);
    }
    buffer_pos = i;
}

static int split_command(char *cmd, char *args[], int max_args) {
    int arg_count = 0;
    int in_arg = 0;
    
    while (*cmd && arg_count < max_args) {
        while (*cmd == ' ' || *cmd == '\t') {
            *cmd++ = '\0';
        }
        
        if (*cmd == '\0') {
            break;
        }
        
        args[arg_count++] = cmd;
        
        while (*cmd && *cmd != ' ' && *cmd != '\t') {
            cmd++;
        }
    }
    
    return arg_count;
}

static void process_command(void) {
    command_buffer[buffer_pos] = '\0';
    
    store_command_in_history(command_buffer);
    
    char cmd_upper[256];
    int i;
    for(i = 0; command_buffer[i]; i++) {
        cmd_upper[i] = command_buffer[i] >= 'a' && command_buffer[i] <= 'z' 
                     ? command_buffer[i] - 32 
                     : command_buffer[i];
    }
    cmd_upper[i] = '\0';  
    
    bool return_to_prompt = true;
    
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
    else if (strcmp_kernel(cmd_upper, "MEMORY") == 0) {
        display_memory();
    }
    else if (strncmp_kernel(cmd_upper, "RM ", 3) == 0) {
        const char* path = command_buffer + 3;
        while (*path == ' ') path++;
        
        if (*path == '\0') {
            brew_str("rm: missing operand\n");
        } else {
            if (!fs_remove_file(path)) {
            }
        }
    }
    else if (strcmp_kernel(cmd_upper, "LS") == 0) {
        brew_str("\n");
        fs_list_directory();
    }
    else if (strncmp_kernel(cmd_upper, "LS ", 3) == 0) {
        brew_str("\n");
        const char* path = command_buffer + 3;
        while (*path == ' ') path++;
        fs_list_directory_at_path(path);
    }
    else if (strcmp_kernel(cmd_upper, "PWD") == 0) {
        brew_str("\n");
        fs_print_working_directory();
    }
    else if (strncmp_kernel(cmd_upper, "CD ", 3) == 0) {
        const char* path = command_buffer + 3;
        while (*path == ' ') path++;
        
        if (*path == '\0') {
            brew_str("cd: missing operand\n");
        } else {
            if (!fs_change_directory(path)) {
                brew_str("cd: cannot access '");
                brew_str(path);
                brew_str("': No such file or directory\n");
            }
        }
    }
    else if (strncmp_kernel(cmd_upper, "MKDIR ", 6) == 0) {
        const char* path = command_buffer + 6;
        while (*path == ' ') path++;
        
        if (*path == '\0') {
            brew_str("mkdir: missing operand\n");
        } else {
            if (!fs_create_directory_at_path(path)) {
                brew_str("mkdir: cannot create directory '");
                brew_str(path);
                brew_str("'\n");
            }
        }
    }
    else if (strncmp_kernel(cmd_upper, "TOUCH ", 6) == 0) {
        const char* path = command_buffer + 6;
        while (*path == ' ') path++;
        
        if (*path == '\0') {
            brew_str("touch: missing operand\n");
        } else {
            if (!fs_create_file_at_path(path)) {
                brew_str("touch: cannot create file '");
                brew_str(path);
                brew_str("'\n");
            }
        }
    }
    else if (strncmp_kernel(cmd_upper, "CAT ", 4) == 0) {
        brew_str("\n");
        const char* path = command_buffer + 4;
        while (*path == ' ') path++;
        
        if (*path == '\0') {
            brew_str("cat: missing operand\n");
        } else {
            size_t size = 0;
            const char* content = fs_read_file_at_path(path, &size);
            if (content) {
                for (size_t i = 0; i < size; i++) {
                    print_char(content[i]);
                }
                brew_str("\n");
            } else {
                brew_str("cat: cannot open '");
                brew_str(path);
                brew_str("': No such file or directory\n");
            }
        }
    }
    else if (strncmp_kernel(cmd_upper, "ECHO ", 5) == 0) {
        const char* args = command_buffer + 5;
        while (*args == ' ') args++;
        
        // Look for redirection operator >
        char redirect_path[256] = {0};
        const char* redirect_pos = args;
        bool found_redirect = false;
        int redirect_idx = 0;
        
        // Find the > character
        while (*redirect_pos) {
            if (*redirect_pos == '>') {
                found_redirect = true;
                redirect_pos++; // skip the >
                while (*redirect_pos == ' ') redirect_pos++; // skip spaces
                // Copy redirect path
                redirect_idx = 0;
                while (*redirect_pos && redirect_idx < 255) {
                    redirect_path[redirect_idx++] = *redirect_pos;
                    redirect_pos++;
                }
                redirect_path[redirect_idx] = '\0';
                break;
            }
            redirect_pos++;
        }
        
        if (found_redirect && redirect_path[0] != '\0') {
            // Write to file
            // Get the text to echo (everything before >)
            char echo_text[256] = {0};
            int text_idx = 0;
            const char* text_ptr = args;
            while (*text_ptr && *text_ptr != '>' && text_idx < 255) {
                echo_text[text_idx++] = *text_ptr;
                text_ptr++;
            }
            echo_text[text_idx] = '\0';
            
            // Trim trailing spaces from echo text
            while (text_idx > 0 && echo_text[text_idx - 1] == ' ') {
                echo_text[--text_idx] = '\0';
            }
            
            if (!fs_write_file_at_path(redirect_path, echo_text, text_idx)) {
                brew_str("echo: cannot write to '");
                brew_str(redirect_path);
                brew_str("'\n");
            }
        } else {
            // Just print to console
            brew_str("\n");
            brew_str(args);
            brew_str("\n");
        }
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
    else if (strcmp_kernel(cmd_upper, "FUCKYOU") == 0) {
        brew_str("\n");
        brew_str("no you");
    }  
    else if (strcmp_kernel(cmd_upper, "SUDO APT UPDATE") == 0) {
        brew_str("\n");
        brew_str("bro what do you think this is? Ubuntu?");
    }  
    else if (strcmp_kernel(cmd_upper, "BEEP") == 0) {
        beep_command();
    }
    else if (strcmp_kernel(cmd_upper, "TXTEDIT") == 0 || (brew_strlen(cmd_upper) > 7 && strncmp_kernel(cmd_upper, "TXTEDIT ", 8) == 0)) {
        if (brew_strlen(command_buffer) > 8) {
            txtedit_run(&command_buffer[8]);
        } else {
            txtedit_run(NULL);
        }
        print_clear();
    }
    else if (strcmp_kernel(cmd_upper, "SHUTDOWN") == 0) {
        shutdown_command();
    }
    else if (strcmp_kernel(cmd_upper, "REBOOT") == 0) {
        reboot_command();
    }
    else if (strcmp_kernel(cmd_upper, "COWSAY") == 0 || (brew_strlen(cmd_upper) > 6 && strncmp_kernel(cmd_upper, "COWSAY ", 7) == 0)) {
        display_cowsay(command_buffer);
    }
        else if (strcmp_kernel(cmd_upper, "BREWER") == 0 || (brew_strlen(cmd_upper) > 6 && strncmp_kernel(cmd_upper, "BREWER ", 7) == 0)) {
            brew_str("\n");
            if (brew_strlen(command_buffer) > 7) {
                char* args[2];
                args[0] = "brewer";  
                args[1] = &command_buffer[7];  
                while (*args[1] == ' ') args[1]++;
                brewer_main(2, args);
            } else {
                brewer_main(1, (char*[]){"brewer"}); 
            }
        }
    else if (shell_handle_command(cmd_upper, command_buffer, (int*)&return_to_prompt)) {
    }
    else if (net_handle_command(cmd_upper, command_buffer, (int*)&return_to_prompt)) {
    }
    else if (strcmp_kernel(cmd_upper, "EXIT") == 0) {
        shutdown_command();
        
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
        brew_str("Brew kernel v3.0\n");
        brew_str("Copyright (C) 2024-2026 boreddevnl.\n");
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

        buffer_pos = 0;
        return_to_prompt = false;
    }
    else if (buffer_pos > 0) {
        brew_str("\nUnknown command. Type HELP for available commands.\n");
    }
    
    if (return_to_prompt) {
        buffer_pos = 0;
        brew_str("\nbrew> ");
    }
}

void kernel_main(void* multiboot_info) {
    print_clear();
    
    print_init_palette();
    init_idt();                      
    pic_init();                           
    irq_init();                            
    timer_init(TIMER_FREQUENCY);           
    
    __asm__ __volatile__("sti");
    sys_memory_init(multiboot_info);
    fs_init();
    init_uptime();
    // these colors might not be accurate since other parts can modify the palette.. (i know this is cursed but i'm lazy and it works)
    print_set_palette_color(1, 0, 113, 255);   // Blue
    print_set_palette_color(2, 245, 194, 45);   // Yellow
    print_set_palette_color(3, 255, 129, 63);   // Orange
    print_set_palette_color(4, 237, 28, 36);    // Red
    print_set_palette_color(5, 163, 73, 164);   // Purple
    print_set_palette_color(6, 108, 198, 74);  // Green
    print_set_palette_color(7, 172, 140, 104);  // Latte
    print_set_palette_color(14, 252, 3, 236); // pink

    // Display sysinfo
        print_clear();
        

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

        // color palette preview
        for (int i = 0; i < 16; i++) {
            print_set_color(i, i);  
            print_char(' ');       
        }
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("\n\n");

                print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("Brew kernel v3.0\n");
        brew_str("Copyright (C) 2024-2026 boreddevnl.\n");
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


    brew_str("Welcome to the Brew kernel!\n");
    print_enable_cursor();  // Enable the hardware cursor

#if AUTO_START_CLI
    in_cli_mode = 1;
    clistart();
    brew_str("brew> ");
    buffer_pos = 0;
#endif

    while (1) {
        net_check_udp_received();
        
        if (check_keyboard()) {
            unsigned char scan_code = read_scan_code();
            
            // special keys
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
                            command_buffer[buffer_pos] = '\0';
                            char cmd_upper[256];
                            int i;
                            for(i = 0; command_buffer[i]; i++) {
                                cmd_upper[i] = command_buffer[i] >= 'a' && command_buffer[i] <= 'z' 
                                           ? command_buffer[i] - 32 
                                           : command_buffer[i];
                            }
                            cmd_upper[i] = '\0';  
                            
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
                        
                        net_check_udp_received();
                    } else if (buffer_pos < sizeof(command_buffer) - 1) {
                        history_current = -1;
                        command_buffer[buffer_pos++] = ascii_char;
                        print_char(ascii_char);
                    }
                }
            }
            
            brewing(10000000); 
        }
    }
}
