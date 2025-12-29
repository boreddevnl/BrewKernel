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
    {"North America", "UTC-10: Honolulu, Papeete", -10, 0},
    {"North America", "UTC-9: Anchorage, Juneau", -9, 0},
    {"North America", "UTC-8: Los Angeles, Vancouver,\n        Seattle, San Francisco", -8, 0},
    {"North America", "UTC-7: Denver, Phoenix,\n        Calgary, Salt Lake City", -7, 0},
    {"North America", "UTC-6: Chicago, Mexico City,\n        Dallas, Houston", -6, 0},
    {"North America", "UTC-5: New York, Toronto,\n        Miami, Ottawa", -5, 0},

    // South America
    {"South America", "UTC-5: Bogota, Lima, Quito", -5, 0},
    {"South America", "UTC-4: Caracas, Santiago, La Paz", -4, 0},
    {"South America", "UTC-3: Buenos Aires,\n        Sao Paulo, Montevideo", -3, 0},

    // Europe
    {"Europe", "UTC+0: London, Dublin,\n        Lisbon, Reykjavik", 0, 0},
    {"Europe", "UTC+1: Paris, Amsterdam, Berlin,\n        Rome, Madrid, Brussels,\n        Vienna, Copenhagen, Oslo, Stockholm", 1, 0},
    {"Europe", "UTC+2: Athens, Bucharest,\n        Helsinki, Sofia, Riga, Tallinn", 2, 0},
    {"Europe", "UTC+3: Kyiv, Moscow,\n        Istanbul, Minsk", 3, 0},

    // Asia
    {"Asia", "UTC+4: Dubai, Abu Dhabi,\n        Muscat, Baku", 4, 0},
    {"Asia", "UTC+5: Karachi, Tashkent,\n        Ashgabat", 5, 0},
    {"Asia", "UTC+5:30: Mumbai, New Delhi,\n         Colombo", 5, 30},
    {"Asia", "UTC+6: Dhaka, Almaty, Thimphu", 6, 0},
    {"Asia", "UTC+7: Bangkok, Jakarta,\n        Hanoi, Phnom Penh", 7, 0},
    {"Asia", "UTC+8: Shanghai, Beijing,\n        Singapore, Kuala Lumpur,\n        Manila, Taipei", 8, 0},
    {"Asia", "UTC+9: Tokyo, Seoul, Pyongyang", 9, 0},

    // Oceania
    {"Oceania", "UTC+10: Sydney, Melbourne,\n         Brisbane, Port Moresby", 10, 0},
    {"Oceania", "UTC+12: Auckland, Wellington,\n         Suva", 12, 0},

    // Africa
    {"Africa", "UTC+0: Accra, Dakar, Monrovia", 0, 0},
    {"Africa", "UTC+1: Lagos, Algiers,\n        Tunis, Casablanca", 1, 0},
    {"Africa", "UTC+2: Cairo, Johannesburg,\n        Harare, Maputo", 2, 0},
    {"Africa", "UTC+3: Nairobi, Addis Ababa,\n        Dar es Salaam, Kampala", 3, 0}
};

static const int num_timezones = sizeof(timezones) / sizeof(timezone);

#endif // TIMEZONES_H