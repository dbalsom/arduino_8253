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
#include <avr/io.h>
#include "arduino_8253.h"

// Tick the PIT.
void pit_clock_tick() {
  SET_CLK_HIGH;
  delayMicroseconds(CLOCK_PIN_HIGH_DELAY);
  SET_CLK_LOW;
  delayMicroseconds(CLOCK_PIN_LOW_DELAY);
}

// Build a command byte given the specified access mode, coutning mode and bcd mode.
u8 build_command_byte(u8 counter, pit_access access, pit_mode mode, bool bcd) {

  u8 byte = 0;

  // Control word format
  //  d7   d6   d5   d4   d3   d2   d1   d0
  // [sc1][sc0][rl1][rl0][ m2][ m1][ m0][bcd]

  byte |= (u8)bcd;
  byte |= ((u8)mode & 0x07) << 1;
  byte |= ((u8)access & 0x03) << 4;
  byte |= ((u8)counter & 0x03) << 6;

  return byte;
}

// Configure the PIT for the specified counter, access mode, coutning mode and bcd mode.
void pit_set_mode(u8 counter, pit_access access, pit_mode mode, bool bcd) {

  u8 byte = build_command_byte(counter, access, mode, bcd);
  pit_write_port(COMMAND, byte);
}

// Write a 16 bit counter value to the specified timer channel.
void pit_write_counter(u8 channel, pit_access access, u16 value) {

  u8 port = (pit_port)(channel + 1);

  switch(access) {
    MSB:
      pit_write_port(port, (u8)((value >> 8) & 0xFF));
      break;
    LSB:
      pit_write_port(port, (u8)(value & 0xFF));
      break;
    LSBMSB:
      pit_write_port(port, (u8)(value & 0xFF));
      // Do we need a clock cycle before next byte or some bigger delay?
      // (Do we need to toggle WR?)
      pit_clock_tick();
      pit_write_port(port, (u8)((value >> 8) & 0xFF));
      break;
    default:
      Serial.println("Invalid access mode specified.");
      break;
  }
}

// Read a 16 bit counter value from the specified timer channel.
void pit_read_counter(u8 channel, pit_access access, u16 value) {

  u8 port = (pit_port)(channel + 1);

  switch (access) {
    MSB:
      pit_write_port(port, (u8)((value >> 8) & 0xFF));
      break;
    LSB:
      pit_write_port(port, (u8)(value & 0xFF));
      break;
    LSBMSB:
      pit_write_port(port, (u8)(value & 0xFF));
      // Do we need a clock cycle before next byte or some bigger delay?
      // (Do we need to toggle WR?)
      pit_clock_tick();
      pit_write_port(port, (u8)((value >> 8) & 0xFF));
      break;
    default:
      Serial.println("Invalid access mode specified.");
      break;
  }
}

// Set the status of the gate input for the specified timer channel.
bool pit_set_gate(u8 channel, bool state) {

  switch (channel) {
    case 0:
      SET_G0(state);
      break;
    case 1:
      SET_G1(state);
      break;
    case 2:
      SET_G2(state);
      break;
    default:
      break;
  }
}

// Read a byte value from the specified port enum.
u8 pit_read_port(pit_port port) {
  pit_set_address(port);
  return pit_read_data_bus();
}

// Write a byte value to the specified port enum.
void pit_write_port(pit_port port, u8 byte) {
  pit_set_address(port);
  pit_write_data_bus(byte);
}

// Read a byte from the 8253's data bus.
// Digital pins 0-7 map cleanly to register D
u8 pit_read_data_bus() {
  pit_set_read();
  // Set PORTD to INPUT
  DDRD = 0;
  delayMicroseconds(PIN_CHANGE_DELAY);
  pit_set_pasv();
  return PORTD;
}

// Write a byte to the 8253's data bus.
// Digital pins 0-7 map cleanly to register D
void pit_write_data_bus(u8 byte) {
  pit_set_write();
  // Set PORTD to OUTPUT
  DDRD = 0xFF;
  delayMicroseconds(PIN_CHANGE_DELAY);
  PORTD = byte;
  pit_set_pasv();
}

// Set the pit to WRITE. RD is active-low.
// WRITE instructions the PIT to expect data from the CPU.
void pit_set_write() {
  SET_WR_LOW;
  SET_RD_HIGH;
}

// Set the PIT to READ. RD is active-low.
// READ instructs the PIT to provide data to the CPU.
void pit_set_read() {
  SET_RD_LOW;
  SET_WR_HIGH;
}

// Set the PIT to neither READ or WRITE.
void pit_set_pasv() {
  SET_RD_HIGH;
  SET_WR_HIGH;
}

// Set the address lines to the PIT to the specified 
// port enum.
void pit_set_address(pit_port port) {
  SET_ADDRESS((u8)port & 0x03);
  delayMicroseconds(PIN_CHANGE_DELAY);
}
