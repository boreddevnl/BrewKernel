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
#include "APPS/doom.h"
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

static int udp_test_active = 0;
static int udp_received_flag = 0;
static ipv4_address_t udp_received_src_ip;
static uint16_t udp_received_src_port;
static uint16_t udp_received_length;
static void udp_echo_callback(const ipv4_address_t* src_ip, uint16_t src_port,
                              const void* data, size_t length) {
    udp_received_flag = 1;
    udp_received_src_ip = *src_ip;
    udp_received_src_port = src_port;
    udp_received_length = (uint16_t)length;
    
    udp_send_packet(src_ip, src_port, 12345, data, length);
}

static void check_udp_received(void) {
    if (udp_received_flag && udp_test_active) {
        udp_received_flag = 0; 
        
        brew_str("\n[UDP] Received ");
        char num[16];
        int pos = 0;
        int n = (int)udp_received_length;
        if (n == 0) {
            num[pos++] = '0';
        } else {
            while (n > 0 && pos < 15) {
                num[pos++] = '0' + (n % 10);
                n /= 10;
            }
            for (int i = 0; i < pos / 2; i++) {
                char tmp = num[i];
                num[i] = num[pos - 1 - i];
                num[pos - 1 - i] = tmp;
            }
        }
        num[pos] = '\0';
        brew_str(num);
        brew_str(" bytes from ");
        for (int i = 0; i < 4; i++) {
            if (i > 0) brew_str(".");
            n = udp_received_src_ip.bytes[i];
            pos = 0;
            if (n >= 100) {
                num[pos++] = '0' + (n / 100);
                n %= 100;
            }
            if (n >= 10 || pos > 0) {
                num[pos++] = '0' + (n / 10);
                n %= 10;
            }
            num[pos++] = '0' + n;
            num[pos] = '\0';
            brew_str(num);
        }
        brew_str(":");
        n = udp_received_src_port;
        pos = 0;
        if (n == 0) {
            num[pos++] = '0';
        } else {
            while (n > 0 && pos < 15) {
                num[pos++] = '0' + (n % 10);
                n /= 10;
            }
            for (int i = 0; i < pos / 2; i++) {
                char tmp = num[i];
                num[i] = num[pos - 1 - i];
                num[pos - 1 - i] = tmp;
            }
        }
        num[pos] = '\0';
        brew_str(num);
        brew_str("\n");
    }
}

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
    else if (strcmp_kernel(cmd_upper, "DOOM") == 0) {
        doom();
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
    else if (strcmp_kernel(cmd_upper, "LS") == 0 || (brew_strlen(cmd_upper) > 2 && strncmp_kernel(cmd_upper, "LS ", 3) == 0)) {
        brew_str("\n");
        if (brew_strlen(command_buffer) > 3) {
            const char* path = &command_buffer[3];
            if (!fs_list_directory_at_path(path)) {
                brew_str("Directory not found: ");
                brew_str(path);
                brew_str("\n");
            }
        } else {
            fs_list_directory();
        }
    }
    else if (strcmp_kernel(cmd_upper, "CAT") == 0 || (brew_strlen(cmd_upper) > 3 && strncmp_kernel(cmd_upper, "CAT ", 4) == 0)) {
        if (brew_strlen(command_buffer) > 4) {
            const char* path = &command_buffer[4];
            while (*path == ' ') path++; 
            if (*path == '\0') {
                brew_str("Usage: CAT <file>\n");
            } else {
                size_t size = 0;
                const char* content = fs_read_file_at_path(path, &size);
                if (!content || size == 0) {
                    brew_str("Error: Could not read file or file is empty\n");
                } else {
                    brew_str("\n");
                    for (size_t i = 0; i < size; i++) {
                        print_char(content[i]);
                    }
                    brew_str("\n");
                }
            }
        } else {
            brew_str("\nUsage: CAT <file>\n");
        }
    }
    else if (strcmp_kernel(cmd_upper, "ECHO") == 0 || (brew_strlen(cmd_upper) > 4 && strncmp_kernel(cmd_upper, "ECHO ", 5) == 0)) {
        brew_str("\n");
        if (brew_strlen(command_buffer) > 5) {
            const char* args = &command_buffer[5];
            while (*args == ' ') args++; 
            
            if (*args == '\0') {
                brew_str("\n");
            } else {
                const char* redirect_pos = args;
                const char* filename = NULL;
                const char* text_start = args;
                const char* text_end = NULL;
                
                while (*redirect_pos) {
                    if (*redirect_pos == '>') {
                        text_end = redirect_pos;
                        while (text_end > text_start && (*(text_end - 1) == ' ' || *(text_end - 1) == '"')) {
                            text_end--;
                        }
                        
                        filename = redirect_pos + 1;
                        while (*filename == '>' || *filename == ' ') filename++;
                        break;
                    }
                    redirect_pos++;
                }
                
                if (filename && *filename != '\0') {
                    if (text_end && text_end > text_start) {
                        char text[512];
                        size_t text_pos = 0;
                        const char* text_ptr = text_start;
                        
                        if (*text_ptr == '"') text_ptr++;
                        
                        while (text_ptr < text_end && text_pos < sizeof(text) - 1) {
                            if (*text_ptr != '"') {
                                text[text_pos++] = *text_ptr;
                            }
                            text_ptr++;
                        }
                        text[text_pos] = '\0';
                        
                        if (fs_write_file_at_path(filename, text, text_pos)) {
                        } else {
                            brew_str("Error: Could not write to file\n");
                        }
                    } else {
                        brew_str("Usage: ECHO \"text\" > <file>\n");
                    }
                } else {
                    const char* print_ptr = args;
                    if (*print_ptr == '"') print_ptr++;
                    
                    while (*print_ptr) {
                        if (*print_ptr == '"' && print_ptr > args) {
                            break;
                        } else if (*print_ptr != '"') {
                            print_char(*print_ptr);
                        }
                        print_ptr++;
                    }
                    brew_str("\n");
                }
            }
        } else {
            brew_str("\n");
        }
    }
    else if (strcmp_kernel(cmd_upper, "TOUCH") == 0 || (brew_strlen(cmd_upper) > 5 && strncmp_kernel(cmd_upper, "TOUCH ", 6) == 0)) {
        if (brew_strlen(command_buffer) > 6) {
            const char* path = &command_buffer[6];
            while (*path == ' ') path++;
            
            brew_str("\n");
            if (*path == '\0') {
                brew_str("Usage: TOUCH <file>\n");
            } else {
                if (fs_create_file_at_path(path)) {
                } else {
                    brew_str("Error: Could not create file\n");
                }
            }
        } else {
            brew_str("\nUsage: TOUCH <file>\n");
        }
    }
    else if (strcmp_kernel(cmd_upper, "CD") == 0) {
        brew_str("\nUsage: CD <directory>\n");
    }
    else if (strcmp_kernel(cmd_upper, "MKDIR") == 0 || (brew_strlen(cmd_upper) > 5 && strncmp_kernel(cmd_upper, "MKDIR ", 6) == 0)) {
        if (brew_strlen(command_buffer) > 6) {
            char* args[16];
            char cmd_copy[256];
            int i;
            for (i = 6; command_buffer[i]; i++) {
                cmd_copy[i-6] = command_buffer[i];
            }
            cmd_copy[i-6] = '\0';
            
            int arg_count = split_command(cmd_copy, args, 16);
            if (arg_count == 0) {
                brew_str("\nUsage: MKDIR <directory> [directory ...]\n");
            } else {
                brew_str("\n");
                bool all_success = true;
                for (int i = 0; i < arg_count; i++) {
                    if (!fs_create_directory_at_path(args[i])) {
                        brew_str("Failed to create directory: ");
                        brew_str(args[i]);
                        brew_str("\n");
                        all_success = false;
                    }
                }
                if (all_success) {
                    brew_str("All directories created successfully\n");
                }
            }
        } else {
            brew_str("\nUsage: MKDIR <directory> [directory ...]\n");
        }
    }
    else if (strcmp_kernel(cmd_upper, "RM") == 0 || (brew_strlen(cmd_upper) > 2 && strncmp_kernel(cmd_upper, "RM ", 3) == 0)) {
        if (brew_strlen(command_buffer) > 3) {
            const char* path = &command_buffer[3];
            while (*path == ' ') path++; 
            
            brew_str("\n");
            if (*path == '\0') {
                brew_str("Usage: RM <file_or_directory>\n");
            } else if (!fs_remove_file(path)) {
            } else {
                brew_str("Successfully removed ");
                brew_str(path);
                brew_str("\n");
            }
        } else {
            brew_str("\nUsage: RM <file_or_directory>\n");
        }
    }
    else if (brew_strlen(cmd_upper) > 3 && strncmp_kernel(cmd_upper, "CD ", 3) == 0) {
        const char* path = &command_buffer[3];
        brew_str("\n");
        if (!fs_change_directory(path)) {
            brew_str("Directory not found: ");
            brew_str(path);
            brew_str("\n");
        }
    }
    else if (strcmp_kernel(cmd_upper, "PWD") == 0) {
        brew_str("\n");
        fs_print_working_directory();
    }
    else if (strcmp_kernel(cmd_upper, "PCISCAN") == 0) {
        brew_str("\nScanning PCI bus...\n");
        pci_device_t devices[32];
        int count = pci_enumerate_devices(devices, 32);
        
        if (count == 0) {
            brew_str("No PCI devices found.\n");
        } else {
            brew_str("Found ");
            char count_str[16];
            int i = 0;
            int num = count;
            if (num == 0) {
                count_str[i++] = '0';
            } else {
                char temp[16];
                int j = 0;
                while (num > 0) {
                    temp[j++] = '0' + (num % 10);
                    num /= 10;
                }
                for (int k = j - 1; k >= 0; k--) {
                    count_str[i++] = temp[k];
                }
            }
            count_str[i] = '\0';
            brew_str(count_str);
            brew_str(" device(s):\n\n");
            
            for (int idx = 0; idx < count; idx++) {
                char bus_str[4], dev_str[4], func_str[4];
                int bus = devices[idx].bus;
                int dev = devices[idx].device;
                int func = devices[idx].function;
                
                bus_str[0] = '0' + (bus / 100);
                bus_str[1] = '0' + ((bus / 10) % 10);
                bus_str[2] = '0' + (bus % 10);
                bus_str[3] = '\0';
                
                dev_str[0] = '0' + (dev / 10);
                dev_str[1] = '0' + (dev % 10);
                dev_str[2] = '\0';
                
                func_str[0] = '0' + func;
                func_str[1] = '\0';
                
                brew_str("  ");
                brew_str(bus_str);
                brew_str(":");
                brew_str(dev_str);
                brew_str(".");
                brew_str(func_str);
                brew_str("  Vendor: 0x");
                
                // Print vendor ID (hex)
                char hex[] = "0123456789ABCDEF";
                brew_str("0000");
                int vendor = devices[idx].vendor_id;
                for (int h = 3; h >= 0; h--) {
                    char hex_digit[2] = {hex[(vendor >> (h * 4)) & 0xF], '\0'};
                    brew_str(hex_digit);
                }
                
                brew_str("  Device: 0x");
                int device_id = devices[idx].device_id;
                for (int h = 3; h >= 0; h--) {
                    char hex_digit[2] = {hex[(device_id >> (h * 4)) & 0xF], '\0'};
                    brew_str(hex_digit);
                }
                
                brew_str("  Class: 0x");
                int class_code = devices[idx].class_code;
                for (int h = 1; h >= 0; h--) {
                    char hex_digit[2] = {hex[(class_code >> (h * 4)) & 0xF], '\0'};
                    brew_str(hex_digit);
                }
                
                brew_str("\n");
            }
        }
    }
    else if (strcmp_kernel(cmd_upper, "NETINFO") == 0) {
        brew_str("\n");
        if (network_is_initialized()) {
            mac_address_t mac;
            ipv4_address_t ip;
            if (network_get_mac_address(&mac) == 0) {
                brew_str("Network: Initialized\n");
                brew_str("MAC Address: ");
                char hex[] = "0123456789ABCDEF";
                for (int i = 0; i < 6; i++) {
                    if (i > 0) brew_str(":");
                    char hex_digit[2] = {hex[(mac.bytes[i] >> 4) & 0xF], '\0'};
                    brew_str(hex_digit);
                    hex_digit[0] = hex[mac.bytes[i] & 0xF];
                    brew_str(hex_digit);
                }
                brew_str("\n");
            } else {
                brew_str("Network: Initialized (MAC address unavailable)\n");
            }
            if (network_get_ipv4_address(&ip) == 0) {
                brew_str("IP Address: ");
                char num[4];
                for (int i = 0; i < 4; i++) {
                    if (i > 0) brew_str(".");
                    // Simple itoa for numbers 0-255
                    int n = ip.bytes[i];
                    int pos = 0;
                    if (n >= 100) {
                        num[pos++] = '0' + (n / 100);
                        n %= 100;
                    }
                    if (n >= 10 || pos > 0) {
                        num[pos++] = '0' + (n / 10);
                        n %= 10;
                    }
                    num[pos++] = '0' + n;
                    num[pos] = '\0';
                    brew_str(num);
                }
                brew_str("\n");
            }
            // Debug statistics
            brew_str("Debug Stats:\n");
            brew_str("  Frames received: ");
            int frames = network_get_frames_received();
            char num[16];
            int pos = 0;
            if (frames == 0) {
                num[pos++] = '0';
            } else {
                int n = frames;
                while (n > 0 && pos < 15) {
                    num[pos++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = 0; i < pos / 2; i++) {
                    char tmp = num[i];
                    num[i] = num[pos - 1 - i];
                    num[pos - 1 - i] = tmp;
                }
            }
            num[pos] = '\0';
            brew_str(num);
            brew_str("\n  UDP packets received: ");
            int udp = network_get_udp_packets_received();
            pos = 0;
            if (udp == 0) {
                num[pos++] = '0';
            } else {
                int n = udp;
                while (n > 0 && pos < 15) {
                    num[pos++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = 0; i < pos / 2; i++) {
                    char tmp = num[i];
                    num[i] = num[pos - 1 - i];
                    num[pos - 1 - i] = tmp;
                }
            }
            num[pos] = '\0';
            brew_str(num);
            brew_str("\n  UDP callbacks called: ");
            int callbacks = network_get_udp_callbacks_called();
            pos = 0;
            if (callbacks == 0) {
                num[pos++] = '0';
            } else {
                int n = callbacks;
                while (n > 0 && pos < 15) {
                    num[pos++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = 0; i < pos / 2; i++) {
                    char tmp = num[i];
                    num[i] = num[pos - 1 - i];
                    num[pos - 1 - i] = tmp;
                }
            }
            num[pos] = '\0';
            brew_str(num);
            brew_str("\n  e1000 receive calls: ");
            int e1000_calls = network_get_e1000_receive_calls();
            pos = 0;
            if (e1000_calls == 0) {
                num[pos++] = '0';
            } else {
                int n = e1000_calls;
                while (n > 0 && pos < 15) {
                    num[pos++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = 0; i < pos / 2; i++) {
                    char tmp = num[i];
                    num[i] = num[pos - 1 - i];
                    num[pos - 1 - i] = tmp;
                }
            }
            num[pos] = '\0';
            brew_str(num);
            brew_str("\n  e1000 receive empty: ");
            int e1000_empty = network_get_e1000_receive_empty();
            pos = 0;
            if (e1000_empty == 0) {
                num[pos++] = '0';
            } else {
                int n = e1000_empty;
                while (n > 0 && pos < 15) {
                    num[pos++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = 0; i < pos / 2; i++) {
                    char tmp = num[i];
                    num[i] = num[pos - 1 - i];
                    num[pos - 1 - i] = tmp;
                }
            }
            num[pos] = '\0';
            brew_str(num);
            brew_str("\n  network_process_frames calls: ");
            int process_calls = network_get_process_calls();
            pos = 0;
            if (process_calls == 0) {
                num[pos++] = '0';
            } else {
                int n = process_calls;
                while (n > 0 && pos < 15) {
                    num[pos++] = '0' + (n % 10);
                    n /= 10;
                }
                for (int i = 0; i < pos / 2; i++) {
                    char tmp = num[i];
                    num[i] = num[pos - 1 - i];
                    num[pos - 1 - i] = tmp;
                }
            }
            num[pos] = '\0';
            brew_str(num);
            brew_str("\n");
        } else {
            brew_str("Network: Not initialized\n");
            brew_str("Use NETINIT to initialize the network card\n");
        }
    }
    else if (strcmp_kernel(cmd_upper, "NETINIT") == 0) {
        brew_str("\n");
        if (network_is_initialized()) {
            brew_str("Network already initialized\n");
        } else {
            brew_str("Initializing network...\n");
            
            pci_device_t device;
            if (pci_find_device(E1000_VENDOR_ID, E1000_DEVICE_ID_82540EM, &device)) {
                uint32_t bar0 = pci_read_config(device.bus, device.device, device.function, 0x10);
                brew_str("Found e1000 device\n");
                brew_str("BAR0: 0x");
                char hex[] = "0123456789ABCDEF";
                for (int h = 7; h >= 0; h--) {
                    char hex_digit[2] = {hex[(bar0 >> (h * 4)) & 0xF], '\0'};
                    brew_str(hex_digit);
                }
                brew_str("\n");
                
                if (bar0 & 1) {
                    brew_str("Device is I/O mapped (not supported)\n");
                } else {
                    uint32_t mmio_base = bar0 & ~0xF;
                    brew_str("MMIO base: 0x");
                    for (int h = 7; h >= 0; h--) {
                        char hex_digit[2] = {hex[(mmio_base >> (h * 4)) & 0xF], '\0'};
                        brew_str(hex_digit);
                    }
                    brew_str("\n");
                    
                    if (mmio_base < 0x40000000 || (mmio_base >= 0xFE800000 && mmio_base < 0xFF000000)) {
                        brew_str("MMIO address is in mapped range\n");
                    } else {
                        brew_str("WARNING: MMIO address is NOT in mapped range!\n");
                        brew_str("This will cause a page fault. Skipping initialization.\n");
                        return;
                    }
                }
            } else {
                brew_str("e1000 device not found\n");
                return;
            }
            
            if (network_init() == 0) {
                brew_str("Network initialized successfully\n");
            } else {
                brew_str("Network initialization failed\n");
            }
        }
    }
    else if (strcmp_kernel(cmd_upper, "UDPTEST") == 0 || 
             (brew_strlen(cmd_upper) > 7 && strncmp_kernel(cmd_upper, "UDPTEST ", 8) == 0)) {
        brew_str("\n");
        if (!network_is_initialized()) {
            brew_str("Network not initialized. Use NETINIT first.\n");
            return_to_prompt = true;
        } else {
            if (!udp_test_active) {
                if (udp_register_callback(12345, udp_echo_callback) == 0) {
                    brew_str("UDP echo server started on port 12345\n");
                    brew_str("Listening for packets...\n");
                    brew_str("Send UDP packets to this IP:port from another machine\n");
                    udp_test_active = 1;
                } else {
                    brew_str("Failed to register UDP callback\n");
                }
            } else {
                brew_str("UDP test already active on port 12345\n");
            }
        }
    }
    else if (strcmp_kernel(cmd_upper, "UDPSEND") == 0 || 
             (brew_strlen(cmd_upper) > 7 && strncmp_kernel(cmd_upper, "UDPSEND ", 8) == 0)) {
        brew_str("\n");
        if (!network_is_initialized()) {
            brew_str("Network not initialized. Use NETINIT first.\n");
            return_to_prompt = true;
        } else {
            // Parse: UDPSEND <ip> <port> <message>
            // Simple parser - expects "UDPSEND 10.0.2.2 12345 hello"
            const char* args = &command_buffer[8];
            while (*args == ' ') args++;
            
            if (*args == '\0') {
                brew_str("Usage: UDPSEND <ip> <port> <message>\n");
                brew_str("Example: UDPSEND 10.0.2.2 12345 hello\n");
            } else {
                // Parse IP
                ipv4_address_t dest_ip;
                int ip_bytes[4] = {0};
                int ip_idx = 0;
                int current = 0;
                const char* p = args;
                while (*p && ip_idx < 4) {
                    if (*p >= '0' && *p <= '9') {
                        current = current * 10 + (*p - '0');
                    } else if (*p == '.' || *p == ' ') {
                        ip_bytes[ip_idx++] = current;
                        current = 0;
                        if (*p == ' ') break;
                    }
                    p++;
                }
                if (ip_idx < 4 && current > 0) {
                    ip_bytes[ip_idx++] = current;
                }
                for (int i = 0; i < 4; i++) {
                    dest_ip.bytes[i] = (uint8_t)ip_bytes[i];
                }
                
                // Skip to port
                while (*p == ' ') p++;
                int port = 0;
                while (*p >= '0' && *p <= '9') {
                    port = port * 10 + (*p - '0');
                    p++;
                }
                
                // Skip to message
                while (*p == ' ') p++;
                const char* message = p;
                size_t msg_len = 0;
                while (message[msg_len] != '\0' && msg_len < 200) msg_len++;
                
                if (msg_len > 0) {
                    if (udp_send_packet(&dest_ip, (uint16_t)port, 54321, message, msg_len) == 0) {
                        brew_str("UDP packet sent successfully\n");
                    } else {
                        brew_str("Failed to send UDP packet\n");
                    }
                } else {
                    brew_str("No message provided\n");
                }
            }
        }
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


    brew_str("Welcome to the Brew kernel!\n");
    print_enable_cursor();  // Enable the hardware cursor

#if AUTO_START_CLI
    in_cli_mode = 1;
    clistart();
    brew_str("brew> ");
    buffer_pos = 0;
#endif

    while (1) {
        check_udp_received();
        
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
                        
                        check_udp_received();
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
