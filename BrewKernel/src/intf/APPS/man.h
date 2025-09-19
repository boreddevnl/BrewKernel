#ifndef APPS_MAN_H
#define APPS_MAN_H

#include "print.h"
#include "keyboard.h"

const char* manual_pages[] = {
    "BrewKernel User Manual",
    "----------------------",
    "",
    "Welcome to the BrewKernel, a simple hobby operating system kernel designed",
    "for x86_64 architecture. This manual provides an overview of the system,",
    "its features, and available commands.",
    "",
    "== System Overview ==",
    "BrewKernel boots into a VGA text mode display (80x25 characters). It",
    "initializes a custom color palette and provides basic keyboard input",
    "handling. The primary user interface is a simple command-line",
    "interface (CLI).",
    "",
    "== Features ==",
    "* VGA Text Mode Driver: Full control over text and background colors.",
    "* PS/2 Keyboard Driver: Handles key presses, including modifier keys",
    "  like Shift.",
    "* Simple CLI: A basic shell to interact with the kernel.",
    "* Command History (sort of): The last entered command remains in the",
    "  buffer but is not yet a full history feature.",
    "",
    "== How to Use the CLI ==",
    "Upon boot, you can type 'CLI' and press Enter to start the command-line",
    "interface. Once in the CLI, you can type commands followed by Enter.",
    "Commands are case-insensitive.",
    "",
    "== Available Commands ==",
    "HELP: Displays a short list of available commands.",
    "",
    "MAN: Shows this detailed user manual. Use UP/DOWN arrow keys to",
    "     scroll and 'q' to quit the manual viewer.",
    "",
    "ABOUT: Displays information about the kernel, including version, build",
    "       date, architecture, and compiler.",
    "",
    "MATH: A simple calculator for basic arithmetic operations (add, subtract,",
    "      multiply, divide) on integers.",
    "",
    "DATE: Displays the current date and time, with an option to select your",
    "      timezone.",
    "",
    ": A simple calculator for basic arithmetic operations (add, subtract,",
    "      multiply, divide) on integers.",
    "",    
    "CLEAR: Clears the entire screen and moves the cursor to the top-left.",
    "",
    "EXIT: Exits the CLI mode and returns to the initial kernel screen.",
    "",
    "LICENSE: Displays the full GNU General Public License v3, under which",
    "         BrewKernel is distributed. Use UP/DOWN to scroll, 'q' to quit.",
    "      The screen will be cleared and the welcome message reprinted.",
    "",
    "--- End of Manual ---"
};
const int manual_num_lines = sizeof(manual_pages) / sizeof(char*);

static void show_manual() {
    int top_line = 0;
    int needs_redraw = 1;

    while (1) {
        if (needs_redraw) {
            print_clear();
            for (int i = 0; i < 24 && (top_line + i) < manual_num_lines; i++) {
                brew_str(manual_pages[top_line + i]);
                brew_str("\n");
            }
            brew_str("-- (Up/Down to scroll, 'q' to quit) --");
            needs_redraw = 0;
        }

        // Wait for a key press before doing anything
        while (!check_keyboard()) { /* Do nothing */ }

        unsigned char sc = read_scan_code(); // Now read the key
        if (sc == 0x48 && top_line > 0) { top_line--; needs_redraw = 1; } // Up Arrow
        else if (sc == 0x50 && (top_line + 24) < manual_num_lines) { top_line++; needs_redraw = 1; } // Down Arrow
        else if (scan_code_to_ascii(sc) == 'q') { break; }

        brewing(10000000); // Delay to handle keyboard typematic rate (key-repeat)
    }
}


#endif // APPS_MAN_H
