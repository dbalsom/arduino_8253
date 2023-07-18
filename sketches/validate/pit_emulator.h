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

#ifndef _PIT_EMULATOR_H
#define _PIT_EMULATOR_H

#include "lib.h"

#define DEBUG_EMU 1

enum PitType {
  kModel8253,
  kModel8254
};

enum TimerMode {
  kInterruptOnTerminalCount,
  kHardwareRetriggerableOneShot,
  kRateGenerator,
  kSquareWaveGenerator,
  kSoftwareTriggeredStrobe,
  kHardwareTriggeredStrobe,
};

enum AccessMode {
  kLatch,
  kLsb,
  kMsb,
  kLsbMsb,
};

enum TimerState {
  kWaitingForReload,
  kWaitingForGate,
  kWaitingForLoadCycle,
  kWaitingForLoadTrigger,
  kReloadNextCycle,
  kGateTriggeredReload,
  kCounting,
  kCountingTriggered,
};

enum LoadState {
  kWaitingForLsb,
  kWaitingForMsb,
  kLoaded
};

enum ReadState {
  kUnlatched,
  kLatched,
  kReadLsb,
  kReadLsbLatched,
};

enum LoadType {
  kInitialLoad,
  kSubsequentLoad
};

class TimerChannel {

  private:
    int c;
    PitType type;
    TimerMode mode;
    AccessMode access_mode;
    TimerState timer_state;
    LoadState load_state;
    LoadType load_type;
    u16 load_mask;
    unsigned long cycles_in_state;
    u16 count_register;
    u16 counting_element;
    bool ce_undefined;
    u16 count_latch;
    bool count_is_latched;

    ReadState read_state;
    bool reload_on_trigger;

    bool bcd_mode;
    bool gate;
    bool armed;
    bool gate_triggered;

    bool output;
    bool output_on_reload;
    bool reload_next_cycle;

  public:
    TimerChannel(PitType type, int channel_number) : type(type), c(channel_number) {

      mode = kInterruptOnTerminalCount;
      access_mode = kLsb;
      timer_state = kWaitingForReload;
      load_state = kWaitingForLsb;
      load_type = kInitialLoad;
      load_mask = 0xFFFF;
      cycles_in_state = 0;
      count_register = 0;
      counting_element = 0;
      ce_undefined = false;
      count_latch = 0;
      count_is_latched = false;

      read_state = kUnlatched;

      reload_on_trigger = false;
      
      bcd_mode = false;
      gate = false;
      armed = false;
      gate_triggered = false;

      output = false;
      output_on_reload = false;
      reload_next_cycle = false;
    }
  
    void setType(PitType pit_type) {
      type = pit_type;
    }

    bool getOutput() {
      return output;
    }
    
    bool is_ce_undefined() {
      return ce_undefined;
    }

    void printState() {
      mprintf(F("m: %d am: %d ts: %d rv: %u ce: %u, cl: %u, l: %d, g: %d, o: %d arm: %d\n"),
        mode, access_mode, timer_state, count_register, counting_element, count_latch, count_is_latched, gate, output, armed);
    }

    void changeTimerState(TimerState new_state) {
      cycles_in_state = 0;
      timer_state = new_state;
    }

    void changeOutputState(bool new_state) {
      // In a full emulator, actions would be taken here on the rising or falling edge of output change, 
      // such triggering interrupts, dma, or speaker output.

      output = new_state;
    }

    void setMode(AccessMode p_access_mode, TimerMode p_timer_mode, bool p_bcd) {
      access_mode = p_access_mode;
      mode = p_timer_mode;
      bcd_mode = p_bcd;

      counting_element = 0;

      // Reset latch
      count_latch = 0;
      count_is_latched = false;

      armed = false;
      ce_undefined = false;

      // Default load mask
      load_mask = 0xFFFF;

      switch(mode) {
        case kInterruptOnTerminalCount:
          changeOutputState(false); // This is the only mode where the output is low on mode set.
          output_on_reload = false; 
          reload_on_trigger = false;
          break;
        case kHardwareRetriggerableOneShot:
          changeOutputState(true);
          output_on_reload = false; 
          reload_on_trigger = true;
          break;
        case kRateGenerator:
          changeOutputState(true);
          output_on_reload = true; // Output in this mode stays high except for one cycle.
          reload_on_trigger = false;
          break;
        case kSquareWaveGenerator:
          changeOutputState(true);
          output_on_reload = true;
          reload_on_trigger = false;
          load_mask = (type == kModel8254) ? 0xFFFE : 0xFFFF; // Only allow even values into counting element on 8254
          break;
        case kSoftwareTriggeredStrobe:
          changeOutputState(true);
          output_on_reload = true; // Output in this mode stays high except for one cycle.
          reload_on_trigger = false;
          break;
        case kHardwareTriggeredStrobe:
          changeOutputState(true);
          output_on_reload = true; // Output in this mode stays high except for one cycle.
          reload_on_trigger = true;
          break;
        default:
          break;
      }

      #if DEBUG_EMU
        mprintf("setMode(): access mode: %d timer mode: %d bcd: %d\n", access_mode, mode, bcd_mode);
      #endif

      // Setting any mode stops counter.
      changeTimerState(kWaitingForReload);
      load_state = kWaitingForLsb;
      load_type = kInitialLoad;
    }

    void sendReloadByte(u8 byte) {
          #if DEBUG_EMU
            mprintf("sendReloadByte(): access mode: %d\n", access_mode);
          #endif     

      switch(access_mode) {
        case kLsb:
          count_register = (u16)byte;

          #if DEBUG_EMU
            mprintf("sendReloadByte(): mode LSB: got lsb: %u\n", count_register);
          #endif          
          completeLoad();
          break;

        case kMsb:
          count_register = (u16)byte << 8;

          #if DEBUG_EMU
            mprintf("sendReloadByte(): mode MSB: got lsb: %u\n", count_register);
          #endif

          completeLoad();
          break;

        case kLsbMsb:
          switch(load_state) {
            case kLoaded:
            case kWaitingForLoadTrigger:
            case kWaitingForLsb:
              count_register = (u16)byte;
      
              #if DEBUG_EMU
                mprintf("sendReloadByte(): mode LSBMSB: got lsb: %u\n", count_register);
              #endif

              // Beginning a load will stop the timer in InterruptOnTerminalCount mode
              // and set output immediately to low.
              if(mode == kInterruptOnTerminalCount) {
                changeOutputState(false);
                changeTimerState(kWaitingForReload);
              }

              load_state = kWaitingForMsb;
              break;

            case kWaitingForMsb:

              count_register |= (u16)byte << 8;

              #if DEBUG_EMU
                mprintf("sendReloadByte(): mode LSBMSB: got msb: %u\n", count_register);
              #endif
              completeLoad();
              break;
          };
          break;
      };
      #if DEBUG_EMU
        mprintf("sendReloadByte(): reload latch is now: %u\n", count_register);
      #endif
    }

    void setGate(bool gate_state) {

      if((gate_state == true) && (gate == false)) {
        // Rising edge of input gate. 
        // This is ignored if we are waiting for a reload value.
        if (timer_state != kWaitingForReload) {
          switch(mode) {
            case kInterruptOnTerminalCount:
              // Rising gate has no effect.
              break;
            case kHardwareRetriggerableOneShot:
              // Rising gate signals reload next tick.
              changeTimerState(kWaitingForLoadCycle);       
              break;
            case kRateGenerator:
              // Rising gate signals reload next tick.
              changeTimerState(kWaitingForLoadCycle);       
              break;
            case kSquareWaveGenerator:
              // Rising gate signals reload next tick.
              changeTimerState(kWaitingForLoadCycle);          
              break;
            case kSoftwareTriggeredStrobe:
              // Rising gate has no effect (?)
              break;
            case kHardwareTriggeredStrobe:
              // Rising gate signals reload next tick.
              changeTimerState(kWaitingForLoadCycle);         
              break;
            default:
              break;
          }
        }
      }
      else {  
        // Falling edge of gate.

        switch(mode) {
          case kInterruptOnTerminalCount:
            // Falling gate has no efect.
            break;
          case kHardwareRetriggerableOneShot:
            // Falling gate has no efect.
            break;
          case kRateGenerator:
            // Falling gate stops count. Output goes high.
            changeTimerState(kWaitingForGate);
            changeOutputState(true);
            break;
          case kSquareWaveGenerator:
            // Falling gate stops count. Output goes high.
            changeTimerState(kWaitingForGate);
            changeOutputState(true);
            break;
          case kSoftwareTriggeredStrobe:
            // Falling gate stops count.
            changeTimerState(kWaitingForGate);
            break;
          case kHardwareTriggeredStrobe:
            // Falling gate has no efect.
            break;
          default:
            break;
        }
      }
      gate = gate_state;
    }

    void latch() {
      count_latch = counting_element;
      count_is_latched = true;
    }

    u16 readCount() {

      u16 count = 0;

      switch(access_mode) {
        case kMsb:
          count = ((u16)readByte() << 8);
          break;
        case kLsb:
          count = readByte() & 0xFF;
          break;
        case kLsbMsb:
          count = readByte() & 0xFF;
          count |= ((u16)readByte() << 8);
          break;
        default:
          count = 0;
          break;
      }
      //#if DEBUG_EMU
      //  mprintf("readCount(): count: %X\n", count);
      //#endif
      return count;
    }

    void changeReadState(ReadState new_state) {
      switch(read_state) {
        case kUnlatched:
          count_is_latched = false;
          break;

        case kLatched:
          count_is_latched = true;
          break;

        case kReadLsb:

          count_is_latched = false;
          break;

        case kReadLsbLatched:

          count_is_latched = true;
          break;
        
        default:
          break;
      }

      read_state = new_state;
    }

    // Read the counter's data port. This returns a byte as appropriate from the either the 
    // counting element or count register.
    u8 readByte() {

      u8 byte = 0;

      switch(read_state) {

        case kUnlatched:
          // Count is not latched. Return counting element directly.
          switch(access_mode) {
            case kMsb:
              byte = (u8)((counting_element >> 8) & 0xFF);
              break;
            case kLsb:
              byte = (u8)(counting_element & 0xFF);
              break;
            case kLsbMsb:
              byte = (u8)(counting_element & 0xFF);
              changeReadState(kReadLsb);
              break;
          }
          break;

        case kReadLsb:
          // Count is not latched, LSB has been read. Return MSB from counting element.
          byte = (u8)((counting_element >> 8) & 0xFF);
          changeReadState(kUnlatched);
          break;

        case kLatched:
          // Count is latched. Return byte from latch as appropriate.
          switch(access_mode) {
            case kMsb:
              byte = (u8)((count_latch >> 8) & 0xFF);
              changeReadState(kUnlatched);
              break;
            case kLsb:
              byte = (u8)(count_latch & 0xFF);
              changeReadState(kUnlatched);
              break;
            case kLsbMsb:
              byte = (u8)(count_latch & 0xFF);
              changeReadState(kReadLsbLatched);
              break;
          }
          break;

        case kReadLsbLatched:
          // Count is latched, LSB has been read. Return MSB from latch.
          byte = (u8)((count_latch >> 8) & 0xFF);
          changeReadState(kUnlatched);
          break;

        default:
          return 0;
          break;
      }
      //#if DEBUG_EMU
      //  mprintf("readByte(): byte: %X\n", byte);
      //#endif
      return byte;
    }

    void tick() {

      if (timer_state == kWaitingForLoadCycle) {

        // Load the current reload value into the counting element.
        counting_element = count_register & load_mask;
        load_state = kLoaded;

        // Start counting.
        changeTimerState(kCounting);

        // Set output state as appropriate for mode.
        changeOutputState(output_on_reload);

        #if DEBUG_EMU
          mprintf(F("tick(): counter reloaded: %u new state: %d output state: %d\n"), counting_element, timer_state, output);
        #endif

        // Counting Element is now defined
        ce_undefined = false;

        // Don't count this tick.
        return;
      }

      if (timer_state == kWaitingForLoadTrigger && cycles_in_state == 0 && armed == true) {
        // First cycle of kWaitingForLoadTrigger. An undefined value is loaded into the counting element.
        #if DEBUG_EMU
          mprintf(F("tick(): undefined value loaded into counting element on kWaitingForLoadTrigger\n"));
        #endif
        counting_element = 0x03;
        ce_undefined = true;
        
        cycles_in_state++;
        // Don't count this tick.
        return;
      }

      if(timer_state == kCounting || timer_state == kCountingTriggered || timer_state == kWaitingForLoadTrigger) {
        switch(mode) {
          case kInterruptOnTerminalCount:
            // Gate controls counting.
            if (gate) {
              count();

              if(counting_element == 0) {
                // Terminal count. Set output high.
                changeOutputState(true);
              }
            }
            break;

          case kHardwareRetriggerableOneShot:
              count();
              if(counting_element == 0) {
                // Terminal count. Set output high if timer is armed.
                if (armed) {
                  changeOutputState(true);
                }
              }  
            break;

          case kRateGenerator:
            // Gate controls counting.
            if (gate) {
              count();
              
              // Output goes low for one clock cycle when count reaches 1.
              // Counter is reloaded next cycle and output goes HIGH.
              if (counting_element == 1) {
                changeOutputState(false);
                output_on_reload = true;
                changeTimerState(kWaitingForLoadCycle);
              }
            }
            break;

          case kSquareWaveGenerator:
            // Gate controls counting.
            if (gate) {

              if((count_register & 1) == 0) {
                // Even reload value. Count decrements by two and reloads on terminal count.
                count2();
                if (counting_element == 0) {
                  changeOutputState(!output); // Toggle output state
                  counting_element = count_register; // Reload counting element
                }
              }
              else {
                if (type == kModel8254) {
                  // On the 8254, odd values are not allowed into the counting element.
                  count2();
                  if (counting_element == 0) {
                    if (output) {
                      // When output is high, reload is delayed one cycle.
                      output_on_reload = !output; // Toggle output state next cycle
                      changeTimerState(kWaitingForLoadCycle); // Reload next cycle                      
                    }
                    else {
                      // Output is low. Reload and update output immediately.
                      changeOutputState(!output); // Toggle output state
                      counting_element = count_register; // Reload counting element
                    }
                  }
                }
                else {
                  // On the 8253, odd values are allowed into the counting element. An odd value
                  // triggers special behavior of output is high.
                  if (output && (counting_element & 1)) {
                    // If output is high and count is odd, decrement by one. The counting element will be even
                    // from now on until counter is reloaded.
                    count();
                  }
                  else if (!output && (counting_element & 1)) {
                    // If output is low and count is odd, decrement by three. The counting element will be even
                    // from now on until counter is reloaded.
                    count3();
                  }
                  else {
                    count2();
                  }

                  if (counting_element == 0) {
                    // Counting element is immediately reloaded and output toggled.
                    changeOutputState(!output);
                    counting_element = count_register;
                  }
                }
              }
            }
            break;

          case kSoftwareTriggeredStrobe:
            // Gate controls counting.
            if (gate) {
              count();
              if (counting_element == 0) {
                changeOutputState(false); // Output goes low for one cycle on terminal count.
              }
              else {
                changeOutputState(true);
              }
            }
            break;

          case kHardwareTriggeredStrobe:
            
            count();
            if (counting_element == 0) {
              changeOutputState(false); // Output goes low for one cycle on terminal count.
            }
            else {
              changeOutputState(true);
            }            
            break;

          default:
            break;
        }
      }

      cycles_in_state++;
    }
  
  private:

    void count() {
      // Decrement and wrap counter appropriately depending on mode.

      if(bcd_mode) {
        if(counting_element == 0) {
          counting_element = 0x9999;
        }
        else {
          // Countdown in BCD...
          if (counting_element & 0x000F) {
            // Ones place is not 0
            counting_element--;
          }
          else if (counting_element & 0x00F0) {
            // Tenths place is not 0, borrow from it
            counting_element -= 0x7; // (0x10 (16) - 7 = 0x09))
          }
          else if (counting_element & 0x0F00) {
            // Hundredths place is not 0, borrow from it
            counting_element -= 0x67; // (0x100 (256) - 0x67 (103) = 0x99)
          }
          else {
            // Borrow from thousandths place
            counting_element -= 0x667; // (0x1000 (4096) - 0x667 () = 0x999)
          }
        }
      }
      else {
        counting_element -= 1; // Counter wraps in binary mode.
      }
      return;
    }

    inline void count2() {
      count();
      count();
    }

    inline void count3() {
      count();
      count();
      count();
    }

    // A load of the reload value has been completed. What happens now depends on whether
    // this is the first load or intial load, and the particular timer mode.
    void completeLoad() {

      switch(load_type) {
        case kInitialLoad:
          // This was the first load. Enter either WaitingForLoadTrigger or WaitingForLoadCycle
          // depending on the flag set by the configured mode.

          if (reload_on_trigger) {
            changeTimerState(kWaitingForLoadTrigger);
          }
          else {
            changeTimerState(kWaitingForLoadCycle);
          }

          #if DEBUG_EMU
            mprintf(F("initial load set timer_state to :%d\n"), timer_state);
          #endif
          // Arm the timer (applicable only to one-shot modes, but doesn't hurt anything to set)
          armed = true;
          // Next load will be a SubsequentLoad
          load_type = kSubsequentLoad;
          break;

        case kSubsequentLoad:
          // This was a subsequent load for an already loaded timer.

          switch(mode) {
            case kInterruptOnTerminalCount:
              // In InterruptOnTerminalCount mode, completing a load will reload that value on the next cycle.
              changeTimerState(kWaitingForLoadCycle);
              break;

            case kSoftwareTriggeredStrobe:
              // In SoftwareTriggeredStrobe mode, completing a load will reload that value on the next cycle.
              changeTimerState(kWaitingForLoadCycle);
              break;
            
            default:
              // Other modes are not reloaded on a subsequent change of the reload value until gate trigger or
              // terminal count. 
              break;
          };
          #if DEBUG_EMU
            mprintf(F("subsequent load set load_state to :%d\n"), load_state);
          #endif
          break;
        default:
          break;
      };
      load_state = kLoaded;
    }
};

class Pit {

  private:
    PitType type;
    unsigned long long pit_cycles;

  public:

    TimerChannel channel[3] = {
      TimerChannel(kModel8253, 0),
      TimerChannel(kModel8253, 1),
      TimerChannel(kModel8253, 2)
    };

    Pit(PitType type) : type(type) {

      if(type != kModel8253) {
        for (int i = 0; i < 3; i++ ) {
          channel[i].setType(type);
        }
      }
    }

    void setMode(u8 c, AccessMode access_mode, TimerMode timer_mode, bool bcd) {
      if (channel < 3) {
        channel[c].setMode(access_mode, timer_mode, bcd);
      }
    }

    void setModeByte(u8 byte) {
      #if DEBUG_EMU
        mprintf("setModeByte(): Received byte %X\n", byte);
      #endif
      bool bcd = (bool)(byte & 0x01);
      TimerMode timer_mode = (TimerMode)((byte >> 1) & 0x07);
      AccessMode access_mode = (AccessMode)((byte >> 4) & 0x03);

      int c = (byte >> 6);

      if(c == 0x03) {
        // Read back command. Supported only on 8254.
        if(type == kModel8254) {
          // Do readback command
        }
      }
      else if(access_mode == kLatch) {
        // Latch command.
        channel[c].latch();
      }
      else {
        channel[c].setMode(access_mode, timer_mode, bcd);
      }
    }

    void tick() {
      for(int i = 0; i < 3; i++ ) {
        channel[i].tick();
      }
    }
};


#endif
