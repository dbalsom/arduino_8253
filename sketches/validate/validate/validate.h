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

#ifndef _VALIDATE_H
#define _VALIDATE_H

#include "arduino_8253.h"

#define SEED 1234

enum FuzzerOp {
  WriteCommand,
  ReadChannel,
  WriteChannel,
  Tick,
  FlipGate
};

#define NUM_FUZZER_OPS 5

#define TEST_CHAN 2
#define FUZZ_CHAN 2

#define PASS " ~~~ PASS ~~~ "
#define FAIL " *** FAIL *** "

// Test declarations
bool test_init();
bool test_mode0();
bool test_mode1();
bool test_mode2();
bool test_mode3();
bool test_mode4();
bool test_mode5();
bool test_bcd();
bool test_rw();
bool test_fuzzer();
bool test_reload_lsb();


bool test_output(int c, bool state);
bool test_counters(int c, pit_access access);
bool test_counters_exact(int c, pit_access access, u16 value);

// Validator declarations
void v_set_mode(u8 c, pit_access access, pit_mode mode, bool bcd );
void v_set_gate(u8 c, bool gate_state );
void v_latch(u8 c);
void v_write_counter(u8 c, pit_access access, u16 value);
void v_ticks(unsigned long ticks);
void v_tick();
bool v_validate_output(u8 c, bool output_state);
bool v_compare_output(u8 c);
bool v_compare_counters(u8 c, pit_access access);



#endif
