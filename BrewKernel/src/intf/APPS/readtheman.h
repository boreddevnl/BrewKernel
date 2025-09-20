#ifndef APPS_NERD_H
#define APPS_NERD_H

#include "print.h"

static void nerd() {
    brew_str("\n");
    brew_str("You read the manual? NERD. you know what?\n");
    brew_str("Fuck you.\n");
    brewing(5000000000);
    
    for(int i = 0; i < 1000000000000000000000000000; i++) {  
        print_set_color(PRINT_INDEX_0, PRINT_INDEX_15); // black on white
        print_clear();
        brewing(500000); 
        
        print_set_color(PRINT_INDEX_15, PRINT_INDEX_0); // white on black
        print_clear();
        brewing(500000);
    }

}

#endif // APPS_NERD_H
