# Arduino8253

This project defines an Arduino interface to the Intel 8253 PIT.

![image](https://github.com/dbalsom/arduino_8253/assets/7229541/7bc59bb0-f8c8-4e10-a09e-63d7a0b0e688)

## Why?

To more deeply investigate the behavior of this chip to enable more accurate emulation for my emulator [MartyPC](https://dbalsom.github.io/martypc/)

## Details

This project utilizes an Arduino UNO R3 or compatible clone. The included Arduino sketch includes a test suite and PIT emulator. These were developed together. 
Results of the test are compared to the emulated results to ensure the emulator is accurate. The PIT emulator was used as the basis for a new PIT emulation implementation
in MartyPC which improved timings and fixed PCM audio playback.

Fritzing and KiCad projects are supplied. This project never moved beyond the breadboard stage, so no PCB is defined in the KiCad project. Feel free to add one.
The Fritzing project shows the breadboard layout with some additional functionality - the output lines are tied to status LEDs, and a hex inverter and octal latch 
are used to capture the data lines from the PIT and display them using LEDs. 

## 8254

This same project can be used to investigate the 8254, which is pin-compatible. The main differences between the 8253 and 8254 are that the 8524 added the read-back command
and enables interleaved reads and writes per timer channel in LSBMSB mode. 

## License

This project is MIT licensed.
