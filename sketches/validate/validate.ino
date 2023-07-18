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

#include <StensTimer.h>

#include "validate.h"
#include "arduino_8253.h"
#include "lib.h"
#include "pit_emulator.h"

#define TEST_AMODE LSB
#define TIMER_SECOND 1

StensTimer* stensTimer = NULL;

unsigned long ms_accumulator = 0;
unsigned long last_ms = 0;

unsigned long pit_cps = 0;
unsigned long pit_cycles = 0;
unsigned long last_pit_cycles = 0;

bool started_test = false;
bool flipflop = false;

PitType pit_type = kModel8253;

Pit emu = Pit(pit_type);

void setup() {
  // Wait for reset after upload.
  delay(150);

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

  // Set up timer
  stensTimer = StensTimer::getInstance();
  stensTimer->setStaticCallback(timerCallback);

  //Timer* myFirstTimer = stensTimer->setInterval(1, 1000);

  // Wait for PIT to initialize
  delayMicroseconds(100);

  randomSeed(SEED);

  //pit_init();
  Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
}

void loop() {
  // put your main code here, to run repeatedly:

  stensTimer->run();
  //clock_tick();

  if(!started_test) {
    started_test = true;

    test_reload_lsb();
    //test_init();
    //test_mode0();
    //test_mode1();
    //test_mode2();
    //test_mode3();
    //test_mode4();
    //test_mode5();
    //test_bcd();
    //test_rw();
    //test_fuzzer();

    if(!started_test) {
      Serial.println("********** BAD STATE ***********");
    }
    Serial.println("~~~~~~~~~~~~~~~~~~~~~~~~~~ END ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~");
  }

}


void test2() {
    Serial.println("Starting tests...");
    started_test = true;
    pit_reset();
    
    Serial.println("Testing InterruptOnTerminalCount...");

    mprintf("ticks: %lu\n", pit_get_ticks());

    SET_G2_HIGH;
    v_set_mode(TEST_CHAN, TEST_AMODE, InterruptOnTerminalCount, false);
    mprintf("ticks: %lu\n", pit_get_ticks());
    v_write_counter(TEST_CHAN, TEST_AMODE, 50);
  
    mprintf("ticks: %lu\n", pit_get_ticks());
    v_ticks(1);
    // Timer should be loaded now. Set gate to count.
    // Read counter
    v_compare_counters(TEST_CHAN, TEST_AMODE);
    
    //v_set_gate(TEST_CHAN, true);

    // Timer should trigger 
    mprintf("ticks: %lu\n", pit_get_ticks());

    if(v_compare_output(TEST_CHAN)) {
      Serial.println("Output matches!");
    }
    else {
      mprintf("Output does not match. Emu: %d Pit: %d\n",  emu.channel[TEST_CHAN].getOutput(), pit_get_output(TEST_CHAN));
    }
    
    for( int k = 0; k < 10; k++ ) {
      //delay(500);
      v_ticks(1);
      // Read counter
      //v_latch(TEST_CHAN);
    
      v_compare_counters(TEST_CHAN, TEST_AMODE);

      mprintf("ticks: %lu\n", pit_get_ticks());
    }
    
    /*
    // Read counter
    v_compare_counters(TEST_CHAN, TEST_AMODE);

    mprintf("ticks: %lu\n", pit_get_ticks());
    //v_ticks(100);
    v_compare_counters(TEST_CHAN, TEST_AMODE);

    mprintf("ticks: %lu\n", pit_get_ticks());
    */

    /*
    v_ticks(10000);
    if(v_compare_output(TEST_CHAN)) {
      Serial.println("Output matches!");
    }
    else {
      snprintf(buf, BUF_LEN, "Output does not match. Emu: %d Pit: %d",  emu.channel[TEST_CHAN].getOutput(), pit_get_output(TEST_CHAN));
      Serial.println(buf);
    }
    */

}

void timerCallback(Timer* timer){
  switch(timer->getAction()) {

    case TIMER_SECOND:
      mprintf( "CPS: %ul\n", pit_cps);
      pit_cps = 0;

      /*
      if(flipflop) {
        //pit_write_counter(0, LSBMSB, 40000);
        //pit_write_counter(1, MSB, 40000);
        pit_write_counter(2, MSB, 40000);
        SET_GATES(0);
        pit_clock_tick();
        SET_GATES(7);
        pit_clock_tick();
        flipflop = !flipflop;
  
      }
      else {
        //pit_write_counter(0, LSBMSB, 20000);
        //pit_write_counter(1, MSB, 20000);
        pit_write_counter(2, MSB, 20000);
        SET_GATES(0);
        pit_clock_tick();
        SET_GATES(7);
        pit_clock_tick();
        flipflop = !flipflop;
      }
      */
      break;

    case 2:
      // Check output states.


      break;
    default:
      break;
  }
}


void clock_tick() {
  pit_clock_tick();
  pit_cps += 1;
}

// Simultaneously set the real PIT and emulated PIT mode.
void v_set_mode(u8 c, pit_access access, pit_mode mode, bool bcd ) {
  
  pit_set_mode(c, access, mode, bcd);

  u8 byte = build_command_byte(c, access, mode, bcd);

  emu.setModeByte(byte);
  //emu.channel[c].setMode((AccessMode)access, (TimerMode)mode, bcd);
}

void v_send_command_byte(u8 byte) {


  emu.setModeByte(byte);
}

// Simultaneously set the real PIT and emulated PIT gate.
void v_set_gate(u8 c, bool gate_state ) {
  // We only support gate #2
  if(c==2) {
    pit_set_gate(c, gate_state);
    emu.channel[c].setGate(gate_state);
  }
}

// Simultaneously send the latch command to the real PIT and emulated PIT.
void v_latch(u8 c) {
  pit_set_latch(c);
  emu.channel[c].latch();
}

// Simultaneously load a reload byte into the real PIT and emulated PIT.
void v_write_counter(u8 c, pit_access access, u16 value) {

  pit_write_counter(c, access, value);

  switch(access) {
    case MSB:
      emu.channel[c].sendReloadByte(value >> 8);
      break;
    case LSB:
      emu.channel[c].sendReloadByte(value & 0xFF);
      break;
    case LSBMSB:
      emu.channel[c].sendReloadByte(value & 0xFF);
      emu.channel[c].sendReloadByte(value >> 8);
      break;
  };
}

// Execute 'ticks' number of ticks.
void v_ticks(unsigned long ticks) {
  for(unsigned long i = 0; i < ticks; i++ ) {
    v_tick();
  }
}

// Simultaneously tick the real PIT and emulated PIT.
void v_tick() {
  
  pit_clock_tick();
  emu.tick();
  pit_cps += 1;
}

bool v_validate_output(u8 c, bool output_state) {

  bool pit_output_correct = false;
  bool pit_output = pit_get_output(c);

  if(pit_output != output_state) {
    mprintf("v_validate_output(): PIT state did not validate. State: %d Expected %d\n", pit_output, output_state);
    return false;
  }

  if(v_compare_output(c)) {
    //mprintf("v_validate_output(): Emulated output matches PIT output.\n");
    return true;
  }
  else {
    mprintf("v_validate_output(): Emulated output does not match PIT output.\n");
    return false;
  }

}

// Check the output of the real PIT vs emulated PIT. Return false if they do not match.
bool v_compare_output(u8 c) {
  
  bool emu_output = emu.channel[c].getOutput();

  bool pit_output = false;
  switch(c) {
    case 0:
      pit_output = READ_OUT0;
      break;

    case 1:
      pit_output = READ_OUT1;
      break;

    case 2:
      pit_output = READ_OUT2;
      break;

    default:
      mprintf(F("Bad counter #\n"));
      return false;
      break;
  };

  mprintf(F("v_compare_output(): emu output: %d pit output: %d\n"), emu_output, pit_output);

  return emu_output == pit_output;
}

bool v_compare_counters(u8 c, pit_access access) {

  u16 emu_counter = emu.channel[c].readCount();
  u16 pit_counter = pit_read_counter(c, access);

  mprintf(F("v_compare_counters(): emu: %u (%X) pit: %u (%X)\n"), emu_counter, emu_counter, pit_counter, pit_counter);

  return emu_counter == pit_counter;
}

