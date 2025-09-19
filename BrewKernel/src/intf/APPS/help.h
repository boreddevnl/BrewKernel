#ifndef APPS_HELP_H
#define APPS_HELP_H

#include "print.h"

static void display_help() {
    brew_str("\nAvailable commands:\n");
    brew_str("  HELP    - Display this help message\n");
    brew_str("  DATE    - Display the current date and time\n");
    brew_str("  EXIT    - Exit CLI mode and return to regular typing\n");
    brew_str("  CLEAR   - Clear the screen\n");
    brew_str("  ABOUT   - Display system information\n");
    brew_str("  MATH    - Perform basic arithmetic\n");
    brew_str("  MAN     - Show the detailed user manual\n");
    brew_str("  LICENSE - Display the GNU GPLv3 license\n");
}

#endif // APPS_HELP_H
