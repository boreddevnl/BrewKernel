/* brewer.c
 * Implementation of the simple .brew interpreter.
 * only function is brew_str, other functions will be implemented later i'm only testing out the possibility of a simple language inside the kernel
 */
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
#include "APPS/brewer.h"
#include "print.h"
#include "file.h"
#include "filesys.h"

#include <stddef.h>
#include <stdbool.h>

static int local_strncmp(const char* a, const char* b, size_t n) {
    for (size_t i = 0; i < n; i++) {
        unsigned char ca = (unsigned char)a[i];
        unsigned char cb = (unsigned char)b[i];
        if (ca != cb) return (int)ca - (int)cb;
        if (ca == '\0') return 0;
    }
    return 0;
}

static int local_strcmp(const char* a, const char* b) {
    size_t i = 0;
    while (a[i] != '\0' && b[i] != '\0') {
        if (a[i] != b[i]) return (unsigned char)a[i] - (unsigned char)b[i];
        i++;
    }
    return (unsigned char)a[i] - (unsigned char)b[i];
}

static const char* local_strrchr(const char* s, int c) {
    const char* last = NULL;
    while (*s) {
        if (*s == (char)c) last = s;
        s++;
    }
    return last;
}

void brewer_interpret(const char* filename) {
    File* file = fs_find_file(filename);
    if (!file) {
        brew_str("Error: File not found.\n");
        return;
    }

    size_t size = 0;
    const char* content = file_get_content(file, &size);
    if (!content || size == 0) {
        brew_str("Error: Could not read file content.\n");
        return;
    }

    for (size_t i = 0; i + 8 < size; i++) {
        if (local_strncmp(&content[i], "brew_str", 8) != 0) continue;

        size_t j = i + 8;
        while (j < size && content[j] != '(') j++;
        if (j >= size) break;

        j++;
        while (j < size && content[j] != '"' && content[j] != ')') j++;
        if (j >= size || content[j] != '"') continue;

        j++; 
        char outbuf[512];
        size_t outp = 0;
        while (j < size && content[j] != '"' && outp + 1 < sizeof(outbuf)) {
            if (content[j] == '\\' && j + 1 < size) {
                char esc = content[j+1];
                if (esc == 'n') { outbuf[outp++] = '\n'; j += 2; continue; }
                if (esc == 't') { outbuf[outp++] = '\t'; j += 2; continue; }
                if (esc == 'r') { outbuf[outp++] = '\r'; j += 2; continue; }
                if (esc == '"') { outbuf[outp++] = '"'; j += 2; continue; }
                if (esc == '\\') { outbuf[outp++] = '\\'; j += 2; continue; }
                j++;
            }
            outbuf[outp++] = content[j++];
        }
        outbuf[outp] = '\0';

    brew_str(outbuf);

        i = j;
    }
}

void brewer_main(int argc, char** argv) {
    if (argc < 2) {
        brew_str("Usage: brewer <filename.brew>\n");
        return;
    }

    const char* filename = argv[1];
    const char* ext = local_strrchr(filename, '.');
    if (!ext || local_strcmp(ext, ".brew") != 0) {
        brew_str("Error: File must have .brew extension\n");
        return;
    }

    brewer_interpret(filename);
}
