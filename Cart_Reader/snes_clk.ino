#include "options.h"
#if defined(enable_NP) || defined(enable_N64) || defined(enable_SNES)

#include <si5351.h>

int32_t readClockOffset() {
  int32_t clock_offset;

  EEPROM_readAnything(CLK_GEN_OFFSET, clock_offset);
  
  return clock_offset;
}

int32_t initializeClockOffset() {
#ifdef clockgen_calibration
  int32_t clock_offset = readClockOffset();
  if (clock_offset > INT32_MIN) {
    i2c_found = clockgen.init(SI5351_CRYSTAL_LOAD_8PF, 0, clock_offset);
  } else {
    i2c_found = clockgen.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
    EEPROM_writeAnything(CLK_GEN_OFFSET, clock_offset);
  }
  return clock_offset;
#else
  // last number is the clock correction factor which is custom for each clock generator
  i2c_found = clockgen.init(SI5351_CRYSTAL_LOAD_8PF, 0, 0);
  return 0;
#endif
}
#endif