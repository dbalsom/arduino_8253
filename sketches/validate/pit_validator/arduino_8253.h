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
#ifndef _ARDUINO_8253_H
#define _ARDUINO_8253_H

#include <avr/io.h>

#define BAUD_RATE 1000000

#define PIT_8254 false

typedef unsigned char u8;
typedef short unsigned int u16;

typedef enum {
  COMMAND = 0,
  DATA0 = 1,
  DATA1 = 2,
  DATA2 = 3,
} pit_port;

typedef enum {
  InterruptOnTerminalCount,
  HardwareRetriggerableOneShot,
  RateGenerator,
  SquareWaveGenerator,
  SoftwareTriggeredStrobe,
  HardwareTriggeredStrobe,
} pit_mode;

typedef enum {
  LATCH,
  MSB,
  LSB,
  LSBMSB,
} pit_access;

// Time in microseconds to wait after setting clock HIGH or LOW
#define CLOCK_PIN_HIGH_DELAY 2
#define CLOCK_PIN_LOW_DELAY 0

// Microseconds to wait after a pin direction change. Without some sort of delay
// a subsequent read/write may fail.
#define PIN_CHANGE_DELAY 4

// ----------------------------- GPIO PINS ----------------------------------//
#define BIT7 0x80
#define BIT6 0x40
#define BIT5 0x20
#define BIT4 0x10
#define BIT3 0x08
#define BIT2 0x04
#define BIT1 0x02
#define BIT0 0x01

// The clock pin should be self explanatory. It runs the PIT.
const int CLK_PIN = 13;
#define SET_CLK_LOW (PORTB &= ~BIT5)
#define SET_CLK_HIGH (PORTB |= BIT5)

// Read pin is active-low. The Read pin determines whether the CPU intends
// to read a value from the PIT's control or data registers.
const int RD_PIN = 17;
#define SET_RD_LOW (PORTC &= ~BIT3)
#define SET_RD_HIGH (PORTC |= BIT3) 

// Write pin is active-low. The Write pin determines whether the CPU intends
// to write a value to the PIT's control or data registers.
const int WR_PIN = 18;
#define SET_WR_LOW (PORTC &= ~BIT4)
#define SET_WR_HIGH (PORTC |= BIT4) 

// The two address lines control whether we are reading/writing to/from the 
// control register or the three channel data registers, in order.
#define SET_A0_LOW (PORTB &= ~BIT0)
#define SET_A0_HIGH (PORTB |= BIT0)

#define SET_A1_LOW (PORTB &= ~BIT1)
#define SET_A1_HIGH (PORTB |= BIT1)

// Since the address lines are adjacent and the first two bits of PORTB we 
// can set them together
#define SET_ADDRESS(x) (PORTB = ((PORTB & ~0x03) | (x & 0x03)))

// The three output pins for each timer channel
#define READ_OUT0 ((PORTC & BIT0) != 0)
#define READ_OUT1 ((PORTC & BIT1) != 0)
#define READ_OUT2 ((PORTC & BIT2) != 0)

// The three gate input pins for each timer channel
#define SET_G0_LOW (PORTB &= ~BIT2)
#define SET_G0_HIGH (PORTB |= BIT2)
#define SET_G0(x) (PORTB = ((PORTB & ~BIT2) | (x & BIT2)))

#define SET_G1_LOW (PORTB &= ~BIT3)
#define SET_G1_HIGH (PORTB |= BIT3)
#define SET_G1(x) (PORTB = ((PORTB & ~BIT3) | (x & BIT3)))

#define SET_G2_LOW (PORTB &= ~BIT4)
#define SET_G2_HIGH (PORTB |= BIT4)
#define SET_G2(x) (PORTB = ((PORTB & ~BIT4) | (x & BIT4)))

// All initial output pins, used to set pin direction on setup
const int OUTPUT_PINS[] = {0,1,2,3,4,5,6,7,8,9,10,11,12,13,17,18};

// All initial input pins, used to set pin direction on setup
const int INPUT_PINS[] = {14,15,16};

// ----------------------------- Functions ----------------------------------//
u8 build_command_byte(u8 counter, pit_access access, pit_mode mode, bool bcd);
void pit_set_mode(u8 counter, pit_access access, pit_mode mode, bool bcd);
void pit_write_counter(u8 channel, pit_access access, u16 value);
bool pit_set_gate(u8 channel, bool state);
u8 pit_read_port(pit_port port);
void pit_write_port(pit_port port, u8 byte);
u8 pit_read_data_bus();
void pit_write_data_bus(u8 byte);
void pit_set_write();
void pit_set_read();
void pit_set_pasv();
void pit_set_address(pit_port port);

#endif