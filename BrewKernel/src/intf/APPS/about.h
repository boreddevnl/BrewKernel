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
#ifndef APPS_ABOUT_H
#define APPS_ABOUT_H

#include "print.h"

static void display_about() {
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
        brew_str("  \      /\n");
        print_set_color(PRINT_INDEX_9, PRINT_INDEX_0); 
        brew_str("   `----'\n\n");
        print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);

      print_set_color(PRINT_INDEX_7, PRINT_INDEX_0);
        brew_str("Brew kernel v3.0\n");
        brew_str("Copyright (C) 2024-2025 boreddevnl.\n");
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

#endif // APPS_ABOUT_H