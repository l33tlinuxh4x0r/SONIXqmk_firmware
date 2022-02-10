/* Copyright 2022 Dimitris Mantzouranis <d3xter93@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "ansi.h"

// readability
#define NA NO_LED

__attribute__ ((weak))
led_config_t g_led_config = { {
    {  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 },
    { 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27 },
    { 28, 29, 30, 31, 32, 33, 34, 35, 36, 37, 38, 39, NA, 40 },
    { 41, NA, 42, 43, 44, 45, 46, 47, 48, 49, 50, 51, NA, 52 },
    { 53, 54, 55, NA, NA, NA, 56, NA, NA, NA, 57, 58, 59, 60 }
}, {
    {0  ,  0}, {12 ,  0}, {25 ,  0}, {37 ,  0}, {50 ,  0}, {62 ,  0}, {75 ,  0}, {87 ,  0}, {100,  0}, {112,  0}, {124,  0}, {137,  0}, {149,  0}, {168,  0},
    {3  , 13}, {19 , 13}, {31 , 13}, {44 , 13}, {56 , 13}, {68 , 13}, {81 , 13}, {93 , 13}, {106, 13}, {118, 13}, {131, 13}, {143, 13}, {155, 13}, {168, 13},
    {5  , 26}, {22 , 26}, {34 , 26}, {47 , 26}, {59 , 26}, {72 , 26}, {84 , 26}, {96 , 26}, {109, 26}, {121, 26}, {134, 26}, {146, 26},            {166, 26},
    {8  , 38},            {28 , 38}, {40 , 38}, {53 , 38}, {65 , 38}, {78 , 38}, {90 , 38}, {103, 38}, {115, 38}, {128, 38}, {140, 38},            {165, 38},
    {2  , 51}, {17 , 51}, {33 , 51},                                  {79 , 51},                                  {126, 51}, {141, 51}, {156, 51}, {171, 51},
}, {
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
    4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,    4,
    4,    4, 4, 4, 4, 4, 4, 4, 4, 4, 4,    4,
    4, 4, 4,          4,          4, 4, 4, 4,
} };