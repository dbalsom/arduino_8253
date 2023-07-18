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
#include <assert.h>
#include "arduino_8253.h"

unsigned long ticks = 0;

// Tick the PIT.
void pit_clock_tick() {
  //Serial.println(" ** tick **");
  SET_CLK_HIGH;
  delayMicroseconds(CLOCK_PIN_HIGH_DELAY);
  SET_CLK_LOW;
  delayMicroseconds(CLOCK_PIN_LOW_DELAY);
  ticks++;
}

// Experiment to see if other things count as a clock pulse.
void pit_fake_clock_tick() {

  SET_WR_LOW;
  delayMicroseconds(CLOCK_PIN_HIGH_DELAY);
  SET_WR_HIGH;
  delayMicroseconds(CLOCK_PIN_LOW_DELAY);
  SET_WR_LOW;
  delayMicroseconds(CLOCK_PIN_HIGH_DELAY);
  SET_WR_HIGH;
  delayMicroseconds(CLOCK_PIN_LOW_DELAY);
}

unsigned long pit_get_ticks() {
  return ticks;
}

// Build a command byte given the specified access mode, coutning mode and bcd mode.
u8 build_command_byte(u8 counter, pit_access access, pit_mode mode, bool bcd) {

  u8 byte = 0;

  // Control word format
  //  d7   d6   d5   d4   d3   d2   d1   d0
  // [sc1][sc0][rl1][rl0][ m2][ m1][ m0][bcd]
  //   counter |  access |  timer_mode  |
  byte |= (u8)bcd;
  byte |= ((u8)mode & 0x07) << 1;
  byte |= ((u8)access & 0x03) << 4;
  byte |= ((u8)counter & 0x03) << 6;

  return byte;
}

void pit_reset() {
  ticks = 0;
  SET_RESET_LOW;
  delay(RESET_DELAY);
  SET_RESET_HIGH;

  mprintf("pit_reset()\n");
}

void pit_init() {
  Serial.println("Setting up channel 0");
  pit_set_mode(0, LSB, InterruptOnTerminalCount, false);
  Serial.println("Setting up channel 1");
  pit_set_mode(1, LSB, InterruptOnTerminalCount, false);
  Serial.println("Setting up channel 2");
  pit_set_mode(2, LSB, InterruptOnTerminalCount, false);
}

// Configure the PIT for the specified counter, access mode, coutning mode and bcd mode.
void pit_set_mode(u8 counter, pit_access access, pit_mode mode, bool bcd) {

  u8 byte = build_command_byte(counter, access, mode, bcd);
  pit_write_port(COMMAND, byte);
}

// Send the latch command
void pit_set_latch(int counter) {

  u8 byte = build_command_byte(counter, LATCH, 0, false); 
  pit_write_port(COMMAND, byte);
}


// Write a 16 bit counter value to the specified timer channel.
void pit_write_counter(u8 channel, pit_access access, u16 value) {

  u8 port = (pit_port)(channel);

  //mprintf("Writing counter %u to port # %d\n", value, port);
  
  switch(access) {
    case LSB:
      pit_write_port(port, (u8)(value & 0xFF));
      break;    
    case MSB:
      pit_write_port(port, (u8)((value >> 8) & 0xFF));
      break;
    case LSBMSB:
      pit_write_port(port, (u8)(value & 0xFF));
      pit_write_port(port, (u8)((value >> 8) & 0xFF));
      break;
    default:
      Serial.println("Invalid access mode specified.");
      break;
  }
}

// Read a 16 bit counter value from the specified timer channel.
u16 pit_read_counter(u8 channel, pit_access access) {

  u8 port = (pit_port)(channel);
  u16 word = 0;

  switch (access) {
    case LSB:
      word = pit_read_port(port);    
      break;
    case MSB:
      word = ((u16)pit_read_port(port) << 8);
      break;
    case LSBMSB:
      word = pit_read_port(port);
      word |= ((u16)pit_read_port(port) << 8);
      break;
    default:
      Serial.println("Invalid access mode specified.");
      break;
  }

  #if DEBUG_COUNT
    mprintf("pit_read_counter(): %u\n", word);
  #endif

  return word;
}

// Set the status of the gate input for the specified timer channel.
bool pit_set_gate(u8 channel, bool state) {

  switch (channel) {
    case 0:
      //SET_G0(state);
      break;
    case 1:
      //SET_G1(state);
      break;
    case 2:
      SET_G2(state);
      break;
    default:
      break;
  }
}

bool pit_get_output(u8 channel) {
    switch (channel) {
    case 0:
      return READ_OUT0;
      break;
    case 1:
      return READ_OUT1;
      break;
    case 2:
      return READ_OUT2;
      break;
    default:
      return false;
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
  u8 byte;
  pit_set_read();
  
  // Set data bus lines to INPUT
  DDRD &= 0x03; // Leave RX/TX alone
  DDRB &= ~0x03; // Only clear LO 2 bits
  
  // Set pullup resistors
  PORTD |= ~0x03; // Leave RX/TX alone
  PORTB |= 0x03; // Set LO 2 bits.  

  delayMicroseconds(BUS_READ_DELAY);

  // Read HO 6 bits from PIND and LO 2 bits from PINB
  byte = (PIND & ~0x03) | (PINB & 0x03);

  /* Debugging stuff. Pull up resistors seem to be needed for stable bus reading.
  for(int i = 0; i < 10; i++ ) {
    byte = (PIND & ~0x03) | (PINB & 0x03);
    mprintf("%d bus read: %02X\n", i, byte);
  }
  */

  
  #if DEBUG_READ
    mprintf("pit_read_data_bus(): %X\n", byte);
  #endif

  pit_set_pasv();

  return byte;
}

// Write a byte to the 8253's data bus.
// Digital pins 0-7 map cleanly to register D
void pit_write_data_bus(u8 byte) {
  pit_set_write();
  // Set data bus lines to OUTPUT
  DDRD |= ~0x03; // Leave RX/TX alone
  DDRB |= 0x03; // Set LO 2 bits.
  
  // Write HO 6 bits to PORTD, leaving RX&TX (0&1) alone
  PORTD = (byte & 0xFC) | (PORTD & 0x03);
  // Write LO 2 bits to PORTB, leaving upper 6 bits alone
  PORTB = (byte & 0x03) | (PORTB & 0xFC);

  // A delay of 4us seems important for the validator to work.
  delayMicroseconds(BUS_WRITE_DELAY);
  pit_set_pasv();

  // Reset the bus
  PORTD &= 0x03;
  PORTB &= 0xFC;

  #if DEBUG_WRITE
    mprintf("pit_write_data_bus(): %X (%X | %X)\n", byte, (PORTD & 0xFC), (PORTB & 0x03));
  #endif
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
  //char buf[100];
  //snprintf(buf, 100, "setting address to %d", port);
  //Serial.println(buf);
  SET_ADDRESS(port);

  assert(((PORTB >> 2) & 0x03) == (u8)port);

  delayMicroseconds(PIN_CHANGE_DELAY);
}
