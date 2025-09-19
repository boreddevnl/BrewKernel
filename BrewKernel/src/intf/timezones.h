#ifndef TIMEZONES_H
#define TIMEZONES_H

// Structure to represent a timezone
typedef struct {
    const char* continent;
    const char* name;
    int offset_h;
    int offset_m;
} timezone;

// Timezone database
static timezone timezones[] = {
    // North America
    {"North America", "UTC-10 (Honolulu)", -10, 0},
    {"North America", "UTC-9 (Anchorage)", -9, 0},
    {"North America", "UTC-8 (Los Angeles)", -8, 0},
    {"North America", "UTC-7 (Denver)", -7, 0},
    {"North America", "UTC-6 (Chicago)", -6, 0},
    {"North America", "UTC-5 (New York)", -5, 0},

    // South America
    {"South America", "UTC-5 (Bogota)", -5, 0},
    {"South America", "UTC-4 (Caracas)", -4, 0},
    {"South America", "UTC-3 (Buenos Aires)", -3, 0},

    // Europe
    {"Europe", "UTC+1 (London)", 1, 0},
    {"Europe", "UTC+2 (Paris)", 2, 0},
    {"Europe", "UTC+3 (Kyiv)", 3, 0},
    {"Europe", "UTC+3 (Moscow)", 3, 0},

    // Asia
    {"Asia", "UTC+5:30 (Mumbai)", 5, 30},
    {"Asia", "UTC+8 (Shanghai)", 8, 0},
    {"Asia", "UTC+9 (Tokyo)", 9, 0},

    // Oceania
    {"Oceania", "UTC+10 (Sydney)", 10, 0},
    {"Oceania", "UTC+12 (Auckland)", 12, 0},

    // Africa
    {"Africa", "UTC+0 (Accra)", 0, 0},
    {"Africa", "UTC+1 (Lagos)", 1, 0},
    {"Africa", "UTC+2 (Cairo)", 2, 0},
    {"Africa", "UTC+3 (Nairobi)", 3, 0},
};

static const int num_timezones = sizeof(timezones) / sizeof(timezone);

#endif // TIMEZONES_H