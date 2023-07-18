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

#include "arduino_8253.h"
#include "pit_emulator.h"

unsigned long ms_accumulator = 0;
unsigned long last_ms = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(BAUD_RATE);

  // Set all output pins to OUTPUT and LOW
  for( int p = 0; p < (sizeof OUTPUT_PINS / sizeof OUTPUT_PINS[0]); p++ ) {
    pinMode(OUTPUT_PINS[p], OUTPUT);
    digitalWrite(OUTPUT_PINS[p], LOW);
  }

  // Set PIT to write mode.
  pit_set_write();

  // Set all input pins to INPUT
  for( int p = 0; p < (sizeof INPUT_PINS / sizeof INPUT_PINS[0]); p++ ) {
    pinMode(INPUT_PINS[p], INPUT);
  }

  // Wait for PIT to initialize
  delayMicroseconds(100);

  // Set Channel #2 mode
  pit_set_mode(2, LSB, HardwareRetriggerableOneShot, false);

}

void loop() {
  // put your main code here, to run repeatedly:

  unsigned long current_ms = millis();

  ms_accumulator += (current_ms - last_ms);

  if (ms_accumulator > 1000) {
    // One second has elapsed

    Serial.println("Second!");

    ms_accumulator -= 1000;
    last_ms = current_ms;
  }

}


void clock_tick() {
  SET_CLK_HIGH;
  delayMicroseconds(CLOCK_PIN_HIGH_DELAY);
  SET_CLK_LOW;
  delayMicroseconds(CLOCK_PIN_LOW_DELAY);
}

