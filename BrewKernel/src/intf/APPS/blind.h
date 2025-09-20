#ifndef APPS_BLIND_H
#define APPS_BLIND_H

#include "print.h"

static void blindme() {
    print_set_color(PRINT_INDEX_0, PRINT_INDEX_15);
    print_clear();
    brew_str("Woah.. is this heaven?\n");
    brew_str("no.\n");


}

#endif // APPS_BLIND_H
