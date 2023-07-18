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
#include <Arduino.h>
#include <assert.h>
#include "lib.h"

void mprintf(const char *fmt, ...) {

    char printf_buffer[MPRINTF_BUF_LEN] = {0};

    va_list args;
    va_start(args, fmt);
    vsnprintf(printf_buffer, MPRINTF_BUF_LEN - 1, fmt, args);
    va_end(args);

    Serial.print(printf_buffer);
}

void mprintf(const __FlashStringHelper *fmt, ...) {

  char fmt_buf[MPRINTF_FMT_LEN] = {0};
  char printf_buffer[MPRINTF_BUF_LEN] = {0};
   
  strncpy_P(fmt_buf, (const char*)fmt, MPRINTF_FMT_LEN-1);
  
  va_list args;
  va_start(args, fmt);
  vsnprintf(printf_buffer, MPRINTF_BUF_LEN - 1, fmt_buf, args);
  va_end(args);

  Serial.print(printf_buffer);
}

void __assert(const char *__func, const char *__file, int __lineno, const char *__sexp) {
    // transmit diagnostic informations through serial link. 
    Serial.println(__func);
    Serial.println(__file);
    Serial.println(__lineno, DEC);
    Serial.println(__sexp);
    Serial.flush();
    // abort program execution.
    abort();
}
