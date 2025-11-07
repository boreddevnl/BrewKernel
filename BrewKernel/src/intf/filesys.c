#include "filesys.h"
#include "file.h"
#include "print.h"
#include <stdbool.h>
#include <stddef.h>

static size_t fs_strlen(const char* str) {
    size_t len = 0;
    while (str[len]) len++;
    return len;
}

static void fs_strcat(char* dest, const char* src) {
    char* d = dest + fs_strlen(dest);
    while ((*d++ = *src++));
}

static char* fs_strrchr(const char* s, int c) {
    const char* found = NULL;
    while (*s) {
        if (*s == (char)c) found = s;
        s++;
    }
    return (char*)found;
}

static int fs_strcmp(const char* s1, const char* s2) {
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return *(const unsigned char*)s1 - *(const unsigned char*)s2;
}

// Global variables
static char current_path[256] = "/";
static File* root_dir = NULL;
static File* current_dir = NULL;

File* fs_internal_resolve_path(const char* path);

void fs_init(void) {
    root_dir = create_file("/", 'd');
    current_dir = root_dir;

    File* bin = create_file("bin", 'd');
    File* home = create_file("home", 'd');
    File* etc = create_file("etc", 'd');
    File* usr = create_file("kernel", 'd');

    if (bin && home && etc) {
        bin->parent = root_dir;
        home->parent = root_dir;
        etc->parent = root_dir;

        bin->children = NULL;
        bin->child_count = 0;

        home->children = NULL;
        home->child_count = 0;

        etc->children = NULL;
        etc->child_count = 0;

        root_dir->children = bin;
        bin->next_sibling = home;
        home->next_sibling = etc;
        etc->next_sibling = NULL;
        root_dir->child_count = 3;
    }
}

void fs_list_directory(void) {
    if (!current_dir) {
        brew_str("Error: Current directory is NULL\n");
        return;
    }

    if (current_dir->child_count == 0) {
        brew_str("Directory is empty\n");
        return;
    }

    File* child = current_dir->children;
    while (child) {
        if (child->type == 'd') {
            brew_str("[DIR]  ");
        } else {
            brew_str("[FILE] ");
        }
        brew_str(child->name);
        brew_str("\n");
        child = child->next_sibling;
    }
}

bool fs_change_directory(const char* path) {
    if (!path) return false;

    File* target = fs_internal_resolve_path(path);
    if (!target) return false;

    current_dir = target;

    if (current_dir == root_dir) {
        current_path[0] = '/';
        current_path[1] = '\0';
        return true;
    }

    const char* parts[64];
    int part_count = 0;
    File* it = current_dir;
    while (it && it != root_dir && part_count < (int)(sizeof(parts)/sizeof(parts[0]))) {
        parts[part_count++] = it->name;
        it = it->parent;
    }

    size_t idx = 0;
    current_path[idx++] = '/';
    for (int p = part_count - 1; p >= 0; p--) {
        const char* name = parts[p];
        for (size_t j = 0; name[j]; j++) {
            current_path[idx++] = name[j];
        }
        if (p > 0) {
            current_path[idx++] = '/';
        }
    }
    current_path[idx] = '\0';
    return true;
}

File* fs_internal_resolve_path(const char* path) {
    if (!path) return NULL;

    const char* p = path;
    File* dir = NULL;
    if (p[0] == '/') {
        dir = root_dir;
        if (p[1] == '\0') return dir;
        p++;
    } else {
        dir = current_dir;
    }

    char component[256];
    size_t pos = 0;
    size_t i = 0;
    while (p[i]) {
        pos = 0;
        while (p[i] && p[i] != '/') {
            if (pos < sizeof(component)-1) component[pos++] = p[i];
            i++;
        }
        component[pos] = '\0';

        if (pos == 0) {
            if (p[i] == '/') i++;
            continue;
        }

        if (pos == 1 && component[0] == '.') {
        } else if (pos == 2 && component[0] == '.' && component[1] == '.') {
            if (dir->parent) dir = dir->parent;
            else return NULL;
        } else {
            File* child = dir->children;
            bool found = false;
            while (child) {
                if (child->type == 'd' && fs_strcmp(child->name, component) == 0) {
                    dir = child;
                    found = true;
                    break;
                }
                child = child->next_sibling;
            }
            if (!found) return NULL;
        }

        if (p[i] == '/') i++;
    }

    return dir;
}

bool fs_list_directory_at_path(const char* path) {
    File* dir = NULL;
    if (!path || path[0] == '\0') {
        dir = current_dir;
    } else {
        dir = fs_internal_resolve_path(path);
    }

    if (!dir) {
        brew_str("Error: Path not found\n");
        return false;
    }

    if (dir->child_count == 0) {
        brew_str("Directory is empty\n");
        return true;
    }

    File* child = dir->children;
    while (child) {
        if (child->type == 'd') {
            brew_str("[DIR]  ");
        } else {
            brew_str("[FILE] ");
        }
        brew_str(child->name);
        brew_str("\n");
        child = child->next_sibling;
    }
    return true;
}

const char* fs_get_working_directory(void) {
    return current_path;
}

void fs_print_working_directory(void) {
    if (!current_path) return;
    brew_str(current_path);
    brew_str("\n");
}

bool fs_create_directory(const char* name) {
    if (!current_dir || !name) return false;

    File* new_dir = create_file(name, 'd');
    if (!new_dir) return false;

    new_dir->parent = current_dir;
    new_dir->children = NULL;
    new_dir->child_count = 0;
    new_dir->next_sibling = NULL;

    if (current_dir->child_count == 0) {
        current_dir->children = new_dir;
    } else {
        File* last = current_dir->children;
        while (last->next_sibling) {
            last = last->next_sibling;
        }
        last->next_sibling = new_dir;
    }
    current_dir->child_count++;
    return true;
}

File* fs_create_file(const char* name) {
    if (!current_dir || !name) return NULL;

    // Check if file already exists
    File* existing = fs_find_file(name);
    if (existing) return NULL;

    File* new_file = create_file(name, 'f');
    if (!new_file) return NULL;

    new_file->parent = current_dir;
    new_file->children = NULL;
    new_file->child_count = 0;
    new_file->next_sibling = NULL;

    if (current_dir->child_count == 0) {
        current_dir->children = new_file;
    } else {
        File* last = current_dir->children;
        while (last->next_sibling) {
            last = last->next_sibling;
        }
        last->next_sibling = new_file;
    }
    current_dir->child_count++;
    return new_file;
}

File* fs_find_file(const char* name) {
    if (!current_dir || !name) return NULL;

    File* child = current_dir->children;
    while (child) {
        if (child->type == 'f' && fs_strcmp(child->name, name) == 0) {
            return child;
        }
        child = child->next_sibling;
    }
    return NULL;
}

bool fs_create_directory_at_path(const char* path) {
    if (!path) return false;

    File* original_dir = current_dir;
    const char* original_path = fs_get_working_directory();
    char original_path_copy[256];
    size_t i;
    for (i = 0; original_path[i]; i++) {
        original_path_copy[i] = original_path[i];
    }
    original_path_copy[i] = '\0';

    if (path[0] == '/') {
        current_dir = root_dir;
        current_path[0] = '/';
        current_path[1] = '\0';
        path++; 
    }

    char component[256];
    size_t start = 0;
    size_t pos = 0;
    bool success = true;

    while (path[start] && success) {
        pos = 0;
        while (path[start + pos] && path[start + pos] != '/') {
            component[pos] = path[start + pos];
            pos++;
        }
        component[pos] = '\0';

        if (pos > 0) {
            if (path[start + pos] == '/') {
                if (!fs_change_directory(component)) {
                    if (!fs_create_directory(component) || !fs_change_directory(component)) {
                        success = false;
                        break;
                    }
                }
            } else {
                success = fs_create_directory(component);
                break;
            }
        }

        start += pos;
        if (path[start] == '/') start++;
    }

    current_dir = original_dir;
    for (i = 0; original_path_copy[i]; i++) {
        current_path[i] = original_path_copy[i];
    }
    current_path[i] = '\0';

    return success;
}

bool fs_remove_file(const char* path) {
    if (!path) return false;

    // Split path into directory and filename
    char dir_path[256];
    const char* last_slash = fs_strrchr(path, '/');
    char name[256];
    File* target_dir;

    if (last_slash) {
        // There's a path component
        size_t dir_len = last_slash - path;
        for (size_t i = 0; i < dir_len; i++) {
            dir_path[i] = path[i];
        }
        dir_path[dir_len] = '\0';
        
        // Get the name part after the last slash
        const char* name_start = last_slash + 1;
        size_t i = 0;
        while (name_start[i]) {
            name[i] = name_start[i];
            i++;
        }
        name[i] = '\0';

        target_dir = fs_internal_resolve_path(dir_path);
    } else {
        // No path component, just filename in current directory
        size_t i = 0;
        while (path[i]) {
            name[i] = path[i];
            i++;
        }
        name[i] = '\0';
        target_dir = current_dir;
    }

    if (!target_dir) return false;

    // Find and remove the file/directory
    File* prev = NULL;
    File* current = target_dir->children;

    while (current) {
        if (fs_strcmp(current->name, name) == 0) {
            // If it's a directory, make sure it's empty
            if (current->type == 'd' && current->child_count > 0) {
                brew_str("Error: Cannot remove non-empty directory\n");
                return false;
            }

            // Remove from linked list
            if (prev) {
                prev->next_sibling = current->next_sibling;
            } else {
                target_dir->children = current->next_sibling;
            }
            target_dir->child_count--;
            return true;
        }
        prev = current;
        current = current->next_sibling;
    }

    brew_str("Error: File or directory not found\n");
    return false;
}

bool fs_create_directories(const char** names, int count) {
    bool all_success = true;
    for (int i = 0; i < count; i++) {
        if (!fs_create_directory(names[i])) {
            all_success = false;
            brew_str("Error creating directory: ");
            brew_str(names[i]);
            brew_str("\n");
        }
    }
    return all_success;
}