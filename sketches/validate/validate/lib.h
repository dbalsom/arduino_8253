/*
    (C)2023 Daniel Balsom
    https://github.com/dbalsom/arduino_8253

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.

*/
#ifndef _MY_LIB_H
#define _MY_LIB_H

#include <Arduino.h>

#define MPRINTF_FMT_LEN 128
#define MPRINTF_BUF_LEN 256

void mprintf(const char *fmt, ...);
void mprintf(const __FlashStringHelper *fmt, ...);

#endif // _MY_LIB_H