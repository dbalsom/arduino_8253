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
#include "validate.h"
#include "arduino_8253.h"
#include "pit_emulator.h"
#include "lib.h"

extern Pit emu;
extern PitType pit_type;

bool test_counters(int c, pit_access access) {

  if(v_compare_counters(TEST_CHAN, access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
    return true;
  }
  else {
    if (emu.channel[c].is_ce_undefined() ){
      mprintf(F(">>> Counters don't match, but emulator reports counter value undefined. Continuing...\n"));
      return true;
    }
    else {
      mprintf(F(">>> Counters don't match. %s\n"), FAIL);
      return false;
    }
  }
}

bool test_counters_exact(int c, pit_access access, u16 value) {

  if(v_compare_counters(TEST_CHAN, access)){
    mprintf(F(">>> Counters match. %s\n"), PASS);
    u16 emu_counter = emu.channel[c].readCount();
    if(emu_counter != value) {
      mprintf(F(">>> Counter value %u not expected value: %u. %s\n"), emu_counter, value, FAIL);
      return false;
    }
    else {
      mprintf(F(">>> Counter value expected. %s\n"), PASS);
    }
    return true;
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }
}

bool test_output(int c, bool state) {

  const char *state_str = (state == true) ?  "HIGH" : "LOW";

  if(v_validate_output(TEST_CHAN, state)) {
    mprintf(F(">>> Output %s: %s\n"), state_str, PASS);
    return true;
  }
  else {
    mprintf(F(">>> Output not %s or output mismatch! %s\n"), state_str, FAIL);
    return false;
  }
}

bool test_init() {
  mprintf(F("Starting test of uninitialized PIT...\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  mprintf(F(">>> Reading counters without mode set.\n"));
  u16 word = 0;

  for(int c = 2; c >= 0; c-- ) {
    word = pit_read_counter(c, LSBMSB);
    mprintf(F("Channel %d, counter value read: %u\n"), c, word);
  }

  return true;
}

bool test_mode0() {
  mprintf(F("Starting test of mode #0, InterruptOnTerminalCount...\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0x80;
  pit_access test_access = LSB;

  mprintf(F(">>> Setting mode. Output should be LOW after mode set.\n"));
  v_set_mode(TEST_CHAN, test_access, InterruptOnTerminalCount, false);

  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F(">>> Writing counter of %d\n"), test_counter);
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F(">>> Reading counters. Counters should be 0 after counter set.\n"));

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  mprintf(F(">>> Ticking once. Counters should be loaded.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  mprintf(F(">>> Ticking once. Counters should not change (GATE==LOW).\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  mprintf(F(">>> Setting gate high.\n"));
  v_set_gate(2, true);

  mprintf(F(">>> Ticking once.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter-1)) return false;

  mprintf(F(">>> Ticking 10.\n"));
  v_ticks(10);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter-11)) return false;

  mprintf(F(">>> Ticking 200.\n"));
  v_ticks(200);

  if(!test_counters_exact(TEST_CHAN, test_access, (test_counter - 211) & 0xFF)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 1000.\n"));
  v_ticks(1000);

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_mode1() {
  mprintf(F("Starting test of mode #1, HardwareRetriggerableOneShot...\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0xFF;
  pit_access test_access = LSB;

  v_set_mode(TEST_CHAN, test_access, HardwareRetriggerableOneShot, false);
  
  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Counters should be 0 after mode set.\n"));

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking once. Counters should still be 0 as reload value not set.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  mprintf(F(">>> Ticking 10. Counters should still be 0 as reload value not set.\n"));
  v_ticks(10);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  mprintf(F(">>> Setting reload value. Counters should still be 0.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters(TEST_CHAN, test_access)) return false;

  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Ticking once. Counter may be a small, undefined value.\n"));
  v_ticks(1);

  emu.channel[TEST_CHAN].printState();

  if(!test_counters(TEST_CHAN, test_access)) return false;

  mprintf(F(">>> Ticking once. Counter should decrement from its initial undefined value.\n"));
  v_ticks(1);

  if(!test_counters(TEST_CHAN, test_access)) return false;

  mprintf(F(">>> Ticking once. Counter should decrement from its initial undefined value.\n"));
  v_ticks(1);

  if(!test_counters(TEST_CHAN, test_access)) return false;

  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Ticking 100. Counter should decrement from its initial undefined value.\n"));
  v_ticks(100);

  emu.channel[TEST_CHAN].printState();

  if(!test_counters(TEST_CHAN, test_access)) return false;

  mprintf(F(">>> Output should still be high.\n"));
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Setting gate HIGH. Counters should still be undefined as reload cycle has not completed.\n"));
  v_set_gate(TEST_CHAN, true);

  mprintf(F(">>> Reading counters after gate set...\n"));

  if(!test_counters(TEST_CHAN, test_access)) return false;
  if(!test_output(TEST_CHAN, true)) return false;  

  mprintf(F(">>> Ticking once. Counters should now be loaded. Output should now be low.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F(">>> Ticking 300. One-shot should trigger and output should be HIGH.\n"));
  v_ticks(300);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Setting reload value. Counters should be unchanged. Output should remain HIGH.\n"));
  v_write_counter(TEST_CHAN, test_access, 0x80);  

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 500. Output should remain HIGH.\n"));
  v_ticks(500);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Toggling Gate between clock cycles. Ticking once. Counters should be reloaded. Output should become LOW.\n"));
  v_set_gate(TEST_CHAN, false);
  delay(100);
  v_set_gate(TEST_CHAN, true);
  v_ticks(1);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }

  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_mode2() {
  mprintf(F("Starting test of mode #2, RateGenerator...\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0xFF;
  pit_access test_access = LSB;

  v_set_mode(TEST_CHAN, test_access, RateGenerator, false);

  if(!test_output(TEST_CHAN, true)) return false;
  
  emu.channel[2].printState();

  mprintf(F(">>> Counters should be 0 after mode set.\n"));

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  mprintf(F(">>> Ticking 10. Counters should still be 0 as reload value not set.\n"));
  v_ticks(10);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;   

  mprintf(F(">>> Setting reload value. Counters should still be 0.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  mprintf(F(">>> Ticking once. Counters should be set.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  mprintf(F(">>> Ticking once. Counters should be unchanged as GATE==LOW.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false; 

  mprintf(F(">>> Setting GATE==HIGH. Ticking 254. Counters should be 2. Output should be high.\n"));
  v_set_gate(TEST_CHAN, true);
  v_ticks(254);

  if(!test_counters_exact(TEST_CHAN, test_access, 2)) return false;     

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking once. Output should go low.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 1)) return false;        

  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F(">>> Ticking once. Output should go high.\n"));
  v_ticks(1);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }      

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 100. Output should remain high.\n"));
  v_ticks(100);

  if(!test_counters_exact(TEST_CHAN, test_access, 155)) return false;    

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_mode3() {
  mprintf(F("Starting test of mode #3, SquareWaveGenerator (even reload value)...\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0x1000;
  pit_access test_access = LSBMSB;

  v_set_mode(TEST_CHAN, test_access, SquareWaveGenerator, false);

  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Counters should be 0 after mode set and output should be HIGH.\n"));

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Setting reload value. Counters should still be 0.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;

  mprintf(F(">>> Ticking once. Counters should load.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  mprintf(F(">>> Ticking once. Counters should be unchanged as GATE==LOW.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;  

  mprintf(F(">>> Setting GATE=HIGH and ticking once. Counter should reload. Output should remain HIGH.\n"));
  v_set_gate(TEST_CHAN, true);
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking once. Counter should update. Output should remain HIGH.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter - 2)) return false;

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 2046. Counter should update to 2. Output should remain high.\n"));
  v_ticks(2046);

  if(!test_counters_exact(TEST_CHAN, test_access, 2)) return false;

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 1. Output should flip LOW.\n"));
  v_ticks(1);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }

  if(!test_output(TEST_CHAN, false)) return false;
  
  mprintf(F("Starting test of mode #3, SquareWaveGenerator (odd reload value)\n"));

  mprintf(F(">>> Resetting PIT. Setting gate HIGH.\n"));

  v_set_gate(TEST_CHAN, true);
  pit_reset();

  test_counter = 0x1001;
  test_access = LSBMSB;

  v_set_mode(TEST_CHAN, test_access, SquareWaveGenerator, false);
  delay(100);

  mprintf(F(">>> Setting reload value. Counters should be 0 and output HIGH.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    return false;
  }

  if(!test_output(TEST_CHAN, true)) return false;

  emu.channel[TEST_CHAN].printState();

  if(pit_type == kModel8253) {
    mprintf(F(">>> MODEL 8253: Ticking once. Counters should be loaded to %u and output HIGH.\n"), test_counter);
    v_ticks(1);

    if(v_compare_counters(TEST_CHAN, test_access)) {
      mprintf(F(">>> Counters match. %s\n"), PASS);
    }
    else {
      mprintf(F(">>> Counters don't match. %s\n"), FAIL);
      //return false;
    }
    if(!test_output(TEST_CHAN, true)) return false;
  }
  
  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Ticking once. Counter be %u and output HIGH.\n"), test_counter - 1);
  v_ticks(1);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    //return false;
  }

  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking once. Counters should decrement by 2 and output HIGH.\n"));
  v_ticks(1);
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    //return false;
  }

  if(!test_output(TEST_CHAN, true)) return false;
  
  mprintf(F(">>> Ticking 2047. Counters should be reloaded and output LOW.\n"));
  v_ticks(2047);
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(v_compare_counters(TEST_CHAN, test_access)) {
    mprintf(F(">>> Counters match. %s\n"), PASS);
  }
  else {
    mprintf(F(">>> Counters don't match. %s\n"), FAIL);
    //return false;
  }

  if(!test_output(TEST_CHAN, false)) return false;

  if(pit_type == kModel8253) {
    mprintf(F(">>> MODEL 8253: Ticking once. Counters should be %u and output LOW.\n"), test_counter - 3);
    v_ticks(1);

    if(v_compare_counters(TEST_CHAN, test_access)) {
      mprintf(F(">>> Counters match. %s\n"), PASS);
    }
    else {
      mprintf(F(">>> Counters don't match. %s\n"), FAIL);
      //return false;
    }

    if(!test_output(TEST_CHAN, false)) return false;
  }

  mprintf(F(">>> Ticking 500 and setting new reload value (0x500). Counter should decrement and output LOW.\n"));
  v_ticks(500);
  v_write_counter(TEST_CHAN, test_access, 0x500);

  if(!test_counters(TEST_CHAN, test_access)) return false;

  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F(">>> Ticking 1547. Counter should reload to new value (0x500) and output HIGH.\n"));
  v_ticks(1547);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 0x500)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_mode4() {
  mprintf(F("Starting test of mode #4, SoftwareTriggeredStrobe\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0xFFFF;
  pit_access test_access = LSBMSB;

  v_set_mode(TEST_CHAN, test_access, SoftwareTriggeredStrobe, false);

  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Counters should be 0 after mode set and output should be HIGH.\n"));

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Setting reload value. Counters should be 0 and output HIGH.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, true)) return false;
  
  mprintf(F(">>> Ticking once. Counter should be loaded and output HIGH.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking once. Counter should be unchanged as GATE==LOW.\n"));
  v_ticks(1);
  
  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;

  mprintf(F(">>> Setting GATE==HIGH and ticking once. Counter should decrement.\n"));
  v_set_gate(TEST_CHAN, true);  
  v_ticks(1);
  
  if(!test_counters_exact(TEST_CHAN, test_access, test_counter-1)) return false;

  mprintf(F(">>> Ticking 65533. Counter should be 1 and output HIGH.\n"));
  v_ticks(65533);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 1)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 1. Counter should be 0 and output LOW.\n"));
  v_ticks(1);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, false)) return false;

  mprintf(F(">>> Ticking 1. Counter should be 65535 and output HIGH.\n"));
  v_ticks(1);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 65535)) return false;
  if(!test_output(TEST_CHAN, true)) return false;  

  mprintf(F(">>> Ticking 100. Counter should be 65435 and output HIGH.\n"));
  v_ticks(100);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 65435)) return false;
  if(!test_output(TEST_CHAN, true)) return false;  

  mprintf(F(">>> Setting new reload value of 1000. Counter should be 65435 and output HIGH.\n"));
  v_write_counter(TEST_CHAN, test_access, 1000);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 65435)) return false;
  if(!test_output(TEST_CHAN, true)) return false;    

  mprintf(F(">>> Ticking once. Counter should be 1000 and output HIGH.\n"));
  v_ticks(1);
  
  if(!test_counters_exact(TEST_CHAN, test_access, 1000)) return false;
  if(!test_output(TEST_CHAN, true)) return false;    

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_mode5() {
  mprintf(F("Starting test of mode #5, HardwareTriggeredStrobe\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0x8000;
  pit_access test_access = LSBMSB;

  v_set_mode(TEST_CHAN, test_access, HardwareTriggeredStrobe, false);

  emu.channel[TEST_CHAN].printState();

  mprintf(F(">>> Counters should be 0 after mode set and output should be HIGH.\n"));

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Setting reload value. Counters should be 0 and output HIGH.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, true)) return false;
  
  mprintf(F(">>> Ticking once. Counter is loaded with a small undefined value and output HIGH.\n"));
  v_ticks(1);

  if(!test_counters(TEST_CHAN, test_access)) return false;
  if(!test_output(TEST_CHAN, true)) return false;  

  mprintf(F(">>> Ticking once. Counter will decrement. Output may go LOW if terminal count reached.\n"));
  v_ticks(1);

  if(!test_counters(TEST_CHAN, test_access)) return false;

  mprintf(F(">>> Setting GATE=HIGH and ticking once. Counters should load. Output should be HIGH.\n"));
  v_set_gate(TEST_CHAN, true);
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 7FFF. Counters should be 1. Output should be HIGH.\n"));
  v_ticks(0x7FFF);

  if(!test_counters_exact(TEST_CHAN, test_access, 1)) return false;
  if(!test_output(TEST_CHAN, true)) return false;

  mprintf(F(">>> Ticking 1. Counters should be 0. Output should be LOW.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, false)) return false;  

  mprintf(F(">>> Ticking 1. Counters should be 65535. Output should be HIGH.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 65535)) return false;
  if(!test_output(TEST_CHAN, true)) return false;  

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_bcd() {
  mprintf(F("Starting test of BCD support using Mode0: InterruptOnTerminalCount\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0xFFFF;
  pit_access test_access = LSBMSB;

  mprintf(F("Setting mode. BCD==TRUE. Counters should be 0, output should be LOW.\n"));
  v_set_mode(TEST_CHAN, test_access, InterruptOnTerminalCount, true);
  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, false)) return false;  

  mprintf(F(">>> Setting reload value to invalid BCD value (FFFF). Counters should be 0 and output remains LOW.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, false)) return false;  

  mprintf(F(">>> Ticking once. Counter should be 65535 (0xFFFF).\n"));
  v_ticks(1);
  if(!test_counters_exact(TEST_CHAN, test_access, 65535)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  mprintf(F(">>> Setting GATE=HIGH and ticking once. Counter should be 65534 (0xFFFE).\n"));
  v_set_gate(TEST_CHAN, true);
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 65534)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  test_counter = 0x9999;
  mprintf(F(">>> Setting reload value to a valid BCD value (9999). Counters should be unchanged and output remains LOW.\n"));
  v_write_counter(TEST_CHAN, test_access, test_counter);

  if(!test_counters_exact(TEST_CHAN, test_access, 65534)) return false;
  if(!test_output(TEST_CHAN, false)) return false;   

  mprintf(F(">>> Ticking once. Since an valid BCD value was provided, the counters should reload.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, test_counter)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  mprintf(F(">>> Ticking once. The counters should decrement.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x9998)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    


  mprintf(F(">>> Ticking 10. The counters should decrement.\n"));
  v_ticks(10);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x9988)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  mprintf(F(">>> Ticking 9987. The counters should become 1. Output should remain LOW.\n"));
  v_ticks(9987);

  if(!test_counters_exact(TEST_CHAN, test_access, 1)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  mprintf(F(">>> Ticking 1. The counters should become 0. Output should go HIGH.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0)) return false;
  if(!test_output(TEST_CHAN, true)) return false;    

  mprintf(F(">>> Ticking 1. The counters should become 0x9999. Output should remain HIGH.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x9999)) return false;
  if(!test_output(TEST_CHAN, true)) return false;    

  mprintf(F("TEST PASSED!\n"));

  mprintf(F(">>> Ticking 1. Output should remain HIGH.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x9998)) return false;
  if(!test_output(TEST_CHAN, true)) return false;    

  mprintf(F(">>> Ticking 15000. Output should remain HIGH.\n"));
  v_ticks(15000);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x4998)) return false;
  if(!test_output(TEST_CHAN, true)) return false;    

  mprintf(F(">>> Setting reload value to invalid BCD value (0x100F). Counter should be unchanged and Output should go LOW.\n"));
  v_write_counter(TEST_CHAN, test_access, 0x100F);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x4998)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  mprintf(F(">>> Ticking once. Counter should be 0x100F and Output should remain LOW.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x100F)) return false;
  if(!test_output(TEST_CHAN, false)) return false;    

  mprintf(F(">>> Ticking once. Counter should be 0x100E and Output should remain LOW.\n"));
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x100E)) return false;
  if(!test_output(TEST_CHAN, false)) return false;   

  mprintf(F(">>> Ticking 0x0F. Counter should be 0x0999 and Output should remain LOW.\n"));
  v_ticks(0x0F);

  if(!test_counters_exact(TEST_CHAN, test_access, 0x0999)) return false;
  if(!test_output(TEST_CHAN, false)) return false;   

  mprintf(F(">>> Setting reload value to invalid BCD value (FFFF) and ticking once. Output should remain LOW.\n"));
  v_write_counter(TEST_CHAN, test_access, 0xFFFF);
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 65535)) return false;
  if(!test_output(TEST_CHAN, false)) return false;  

  mprintf(F(">>> Ticking 10. Output should remain LOW.\n"));
  v_write_counter(TEST_CHAN, test_access, 0xFFFF);
  v_ticks(10);

  if(!test_counters_exact(TEST_CHAN, test_access, 65526)) return false;
  if(!test_output(TEST_CHAN, false)) return false;  

  mprintf(F("TEST PASSED!\n"));
  return true;
}

bool test_rw() {
  mprintf(F("Starting test of interleaved RW using Mode0: InterruptOnTerminalCount\n"));

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate LOW\n"));
  v_set_gate(TEST_CHAN, false);
  pit_reset();

  u16 test_counter = 0xBEEF;
  pit_access test_access = LSBMSB;
  u8 emu_lsb_byte = 0;
  u8 pit_lsb_byte = 0;
  u8 msb_byte = 0;

  mprintf(F("Setting mode 0, LSBMSB. Setting counter to 0xBEEF, ticking once to set. Output should be LOW.\n"));
  v_set_mode(TEST_CHAN, test_access, InterruptOnTerminalCount, false);
  v_write_counter(TEST_CHAN, test_access, test_counter);
  v_ticks(1);

  if(!test_counters_exact(TEST_CHAN, test_access, 0xBEEF)) return false;
  if(!test_output(TEST_CHAN, false)) return false;  

  mprintf(F("Latching count...\n"));
  v_latch(TEST_CHAN);

  mprintf(F("Reading LSB in LSBMSB mode.\n"));

  pit_lsb_byte = pit_read_counter(TEST_CHAN, LSB);
  emu_lsb_byte = emu.channel[TEST_CHAN].readByte();

  mprintf(F("Read emu byte: %X, pit byte: %X\n"), emu_lsb_byte, pit_lsb_byte);
  if (emu_lsb_byte != pit_lsb_byte) {
    mprintf(F("Bytes don't match! %s"), FAIL);
    return false;
  }

  mprintf(F("Writing a byte in the middle of LSBMSB read.\n"));
  v_write_counter(TEST_CHAN, LSB, 0xAA);

  // On the 8253, writing a byte to a channel's count register seems to reset the 
  // read state back to LSB.
  mprintf(F("Reading another byte to see what we get.\n"));
  pit_lsb_byte = pit_read_counter(TEST_CHAN, LSB);
  emu_lsb_byte = emu.channel[TEST_CHAN].readByte();

  mprintf(F("Read emu byte: %X, pit byte: %X\n"), emu_lsb_byte, pit_lsb_byte);
  if (emu_lsb_byte != pit_lsb_byte) {
    mprintf(F("Bytes don't match! %s\n"), FAIL);
    //return false;
  }  

  mprintf(F("Reading another byte to see what we get.\n"));
  pit_lsb_byte = pit_read_counter(TEST_CHAN, LSB);
  emu_lsb_byte = emu.channel[TEST_CHAN].readByte();

  mprintf(F("Read emu byte: %X, pit byte: %X\n"), emu_lsb_byte, pit_lsb_byte);
  if (emu_lsb_byte != pit_lsb_byte) {
    mprintf(F("Bytes don't match! %s\n"), FAIL);
    //return false;
  }

  mprintf(F("Writing a byte of count register in LSBMSB mode (0xAA).\n"));
  v_write_counter(TEST_CHAN, LSB, 0xAA);

  mprintf(F("Reading a byte to see what we get.\n"));
  pit_lsb_byte = pit_read_counter(TEST_CHAN, LSB);
  emu_lsb_byte = emu.channel[TEST_CHAN].readByte();

  mprintf(F("Read emu byte: %X, pit byte: %X\n"), emu_lsb_byte, pit_lsb_byte);
  if (emu_lsb_byte != pit_lsb_byte) {
    mprintf(F("Bytes don't match! %s\n"), FAIL);
    //return false;
  }

  mprintf(F("Reading another byte to see what we get.\n"));
  pit_lsb_byte = pit_read_counter(TEST_CHAN, LSB);
  emu_lsb_byte = emu.channel[TEST_CHAN].readByte();

  mprintf(F("Read emu byte: %X, pit byte: %X\n"), emu_lsb_byte, pit_lsb_byte);
  if (emu_lsb_byte != pit_lsb_byte) {
    mprintf(F("Bytes don't match! %s\n"), FAIL);
    //return false;
  }
}

bool test_reload_lsb() {

  u16 test_counter = 0x0002;
  u16 cur_counter = 0;
  pit_access test_access = LSBMSB;

  mprintf(F(">>> Resetting PIT\n"));
  mprintf(F(">>> Gate HIGH\n"));
  v_set_gate(TEST_CHAN, true);
  pit_reset();

  mprintf(F("Setting mode 2, LSBMSB. Setting counter to 0x0002, ticking once to set. Output should be HIGH.\n"));
  v_set_mode(TEST_CHAN, test_access, RateGenerator, false);
  v_write_counter(TEST_CHAN, test_access, test_counter);
  v_ticks(2);

  mprintf(F("Reading timer value in LSBMSB mode.\n"));
  cur_counter  = pit_read_counter(TEST_CHAN, LSBMSB);
  mprintf(F("New timer value is: %X\n"), cur_counter);

  mprintf(F("Writing one byte 0xFF in LSBMSB mode.\n"));
  v_write_counter(TEST_CHAN, LSB, 0xFF);

  mprintf(F("Ticking twice to roll over counter.\n"));
  v_ticks(2);

  mprintf(F("Reading timer value in LSBMSB mode.\n"));
  cur_counter = pit_read_counter(TEST_CHAN, LSBMSB);

  mprintf(F("New timer value is: %X\n"), cur_counter);

  mprintf(F("Writing one byte 0xFF in LSBMSB mode.\n"));
  v_write_counter(TEST_CHAN, LSB, 0xFF);

  mprintf(F("Ticking twice...\n"));
  v_ticks(2);   

  mprintf(F("Reading timer value in LSBMSB mode.\n"));
  cur_counter = pit_read_counter(TEST_CHAN, LSBMSB);

  mprintf(F("New timer value is: %X\n"), cur_counter);  
}


bool test_fuzzer() {

  unsigned long fuzz_ct = 0;

  int fuzz_op = 0;
  bool gate_status = false;
  u8 fuzz_byte = 0;
  u8 emu_byte = 0;
  u8 pit_byte = 0;
  u16 fuzz_ticks = 0;
  bool fuzz_error = false;

  // Set initial gate state
  v_set_gate(FUZZ_CHAN, gate_status);
  pit_reset();

  // Set initial mode and counter for channel.
  v_set_mode(FUZZ_CHAN, LSBMSB, InterruptOnTerminalCount, false);
  v_write_counter(FUZZ_CHAN, LSBMSB, 0xFFFF);

  delay(1000);

  mprintf(F("FUZZER: Beginning fuzzer!\n"));
  while (fuzz_ct < 10000 && !fuzz_error) {

    fuzz_op = (FuzzerOp)random(NUM_FUZZER_OPS + 1);

    switch (fuzz_op) {

      case WriteCommand:
        // Generate a random command byte
        fuzz_byte = random(256);
        // Mask off channel select
        fuzz_byte &= 0x3F;
        // Set channel to FUZZ_CHAN
        fuzz_byte |= (FUZZ_CHAN << 6);

        mprintf(F("FUZZER: Writing %X to command port...\n"), fuzz_byte);
        pit_write_port(COMMAND, fuzz_byte);
        emu.setModeByte(fuzz_byte);
        break;

      case ReadChannel:
        // Read a byte from the FUZZ_CHAN and compare.
        mprintf(F("FUZZER: Reading from data channel...\n"));

        emu_byte = emu.channel[FUZZ_CHAN].readByte();
        pit_byte = pit_read_port(FUZZ_CHAN);
        
        if(emu_byte != pit_byte) {
          // Allow a difference if the emulator says we are in undefined mode.
          if (emu.channel[FUZZ_CHAN].is_ce_undefined()) {
            mprintf(F("Bytes read from data channel differ, but emulator reports undefined status. Continuing.\n"));
          }
          else {
            mprintf(F("Bytes read from data channel differ! emu: %X pit: %X\n"), emu_byte, pit_byte);
            emu.channel[FUZZ_CHAN].printState();
            fuzz_error = true;
          }
        }
        break;

      case WriteChannel:
        // Generate a random data byte
        fuzz_byte = random(256);
        mprintf(F("FUZZER: Writing %X to data channel...\n"), fuzz_byte);
        emu.channel[FUZZ_CHAN].sendReloadByte(fuzz_byte);
        pit_write_port(FUZZ_CHAN, fuzz_byte);
        break;
      
      case Tick:
        // Generate a random number of ticks.
        
        fuzz_ticks = random(0xFFFF);
        mprintf(F("FUZZER: Ticking %X...\n"), fuzz_ticks);
        v_ticks(fuzz_ticks);
        mprintf(F("FUZZER: Ticking %X...\n"), fuzz_ticks);
        v_ticks(fuzz_ticks); // Do it twice, to make wrapping counter more likely.

        break;

      case FlipGate:
        
        // Change the gate status
        gate_status = !gate_status;
        v_set_gate(FUZZ_CHAN, gate_status);
        break;

      default:
        mprintf(F("Invalid fuzzer op!\n"));
        break;
    }

    // Check output status
    if(!v_compare_output(FUZZ_CHAN)) {
      // Output didn't match, but allow this if emulator reports undefined state
      if (emu.channel[FUZZ_CHAN].is_ce_undefined()) {
        mprintf(F("Bytes read from data channel differ, but emulator reports undefined status. Continuing.\n"));
      }
      else {
        mprintf(F("Output states differ!\n"));
        fuzz_error = true;
      }
    }
  }
  
}
