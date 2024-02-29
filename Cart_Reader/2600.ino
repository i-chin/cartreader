//******************************************
// ATARI 2600 MODULE
//******************************************
#if defined(enable_2600)
// Atari 2600
// Cartridge Pinout
// 24P 2.54mm pitch connector
//
//                            LABEL SIDE
//
//         GND +5V  A8  A9  A11 A10 A12 D7  D6  D5  D4  D3
//       +--------------------------------------------------+
//       |  24  23  22  21  20  19  18  17  16  15  14  13  |
// LEFT  |                                                  | RIGHT
//       |   1   2   3   4   5   6   7   8   9  10  11  12  |
//       +--------------------------------------------------+
//          A7  A6  A5  A4  A3  A2  A1  A0  D0  D1  D2  GND
//
//                           BOTTOM SIDE

// Cart Configurations
// Format = {mapper,romsize}
static const byte PROGMEM a2600mapsize[] = {
  0x20, 0,  // 2K
  0x3F, 2,  // Tigervision 8K
  0x40, 1,  // 4K [DEFAULT]
  0xC0, 0,  // "CV" Commavid 2K
  0xD0, 2,  // "DPC" Pitfall II 10K
  0xE0, 2,  // Parker Bros 8K
  0xE7, 4,  // M-Network 16K
  0xF0, 6,  // Megaboy 64K
  0xF4, 5,  // Atari 32K
  0xF6, 4,  // Atari 16K
  0xF8, 2,  // Atari 8K
  0xFA, 3,  // CBS RAM Plus 12K
  0xFE, 2,  // Activision 8K
  0xF9, 2,  // "TP" Time Pilot 8K
  0x0A, 2,  // "UA" UA Ltd 8K
};

byte a2600mapcount = (sizeof(a2600mapsize) / sizeof(a2600mapsize[0])) / 2;
byte a2600mapselect;
int a2600index;

byte a2600[] = { 2, 4, 8, 12, 16, 32, 64 };
byte a2600mapper = 0;
byte new2600mapper;
byte a2600size;
byte e7size;

// EEPROM MAPPING
// 07 MAPPER
// 08 ROM SIZE

//******************************************
//  Menu
//******************************************
// Base Menu
static const char a2600MenuItem1[] PROGMEM = "Select Cart";
static const char a2600MenuItem2[] PROGMEM = "Read ROM";
static const char a2600MenuItem3[] PROGMEM = "Set Mapper";
static const char* const menuOptions2600[] PROGMEM = { a2600MenuItem1, a2600MenuItem2, a2600MenuItem3, string_reset2 };

void setup_2600() {
  // Request 5V
  setVoltage(VOLTS_SET_5V);

  // Set Address Pins to Output
  // Atari 2600 uses A0-A12 [A13-A23 UNUSED]
  //A0-A7
  DDRF = 0xFF;
  //A8-A15
  DDRK = 0xFF;
  //A16-A23
  DDRL = 0xFF;

  // Set Control Pins to Output [UNUSED]
  //       ---(PH0)   ---(PH1)   ---(PH3)   ---(PH4)   ---(PH5)   ---(PH6)
  DDRH |= (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);

  // Set TIME(PJ0) to Output (UNUSED)
  DDRJ |= (1 << 0);

  // Set Pins (D0-D7) to Input
  DDRC = 0x00;

  // Setting Control Pins to HIGH [UNUSED]
  //       ---(PH0)   ---(PH1)   ---(PH3)   ---(PH4)   ---(PH5)   ---(PH6)
  PORTH |= (1 << 0) | (1 << 1) | (1 << 3) | (1 << 4) | (1 << 5) | (1 << 6);

  // Set Unused Data Pins (PA0-PA7) to Output
  DDRA = 0xFF;

  // Set Unused Pins HIGH
  PORTA = 0xFF;
  PORTL = 0xFF;       // A16-A23
  PORTJ |= (1 << 0);  // TIME(PJ0)

  checkStatus_2600();
  strcpy(romName, "ATARI");

  mode = mode_2600;
}

void a2600Menu() {
  convertPgm(menuOptions2600, 4);
  uint8_t mainMenu = question_box(F("ATARI 2600 MENU"), menuOptions, 4, 0);

  switch (mainMenu) {
    case 0:
      // Select Cart
      setCart_2600();
      wait();
      setup_2600();
      break;

    case 1:
      // Read ROM
      sd.chdir("/");
      readROM_2600();
      sd.chdir("/");
      break;

    case 2:
      // Set Mapper
      setMapper_2600();
      checkStatus_2600();
      break;

    case 3:
      // reset
      resetArduino();
      break;
  }
}

//******************************************
// READ CODE
//******************************************

uint8_t readData_2600(uint16_t addr)  // Add Input Pullup
{
  PORTF = addr & 0xFF;         // A0-A7
  PORTK = (addr >> 8) & 0xFF;  // A8-A12
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  DDRC = 0x00;   // Set to Input
  PORTC = 0xFF;  // Input Pullup
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  uint8_t ret = PINC;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  return ret;
}

void readSegment_2600(uint16_t startaddr, uint16_t endaddr) {
  for (uint16_t addr = startaddr; addr < endaddr; addr += 512) {
    for (int w = 0; w < 512; w++) {
      uint8_t temp = readData_2600(addr + w);
      sdBuffer[w] = temp;
    }
    myFile.write(sdBuffer, 512);
  }
}

void readSegmentF8_2600(uint16_t startaddr, uint16_t endaddr, uint16_t bankaddr) {
  for (uint16_t addr = startaddr; addr < endaddr; addr += 512) {
    for (int w = 0; w < 512; w++) {
      if (addr > 0x1FF9)  // SET BANK ADDRESS FOR 0x1FFA-0x1FFF
        readData_2600(bankaddr);
      uint8_t temp = readData_2600(addr + w);
      sdBuffer[w] = temp;
    }
    myFile.write(sdBuffer, 512);
  }
}

void writeData_2600(uint16_t addr, uint8_t data) {
  PORTF = addr & 0xFF;         // A0-A7
  PORTK = (addr >> 8) & 0xFF;  // A8-A12
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  DDRC = 0xFF;  // Set to Output
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  PORTC = data;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  DDRC = 0x00;  // Reset to Input
}

void writeData3F_2600(uint16_t addr, uint8_t data) {
  PORTF = addr & 0xFF;         // A0-A7
  PORTK = (addr >> 8) & 0xFF;  // A8-A12
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  DDRC = 0xFF;  // Set to Output
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  PORTC = data;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  // Address (0x1000);
  PORTF = 0x00;  // A0-A7
  PORTK = 0x10;  // A8-A12
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;
  NOP;

  DDRC = 0x00;  // Reset to Input
}

// E7 Mapper Check - Check Bank for FFs
boolean checkE7(int bank) {
  writeData_2600(0x1800, 0xFF);
  readData_2600(0x1FE0 + bank);
  uint32_t testdata = (readData_2600(0x1000) << 24) | (readData_2600(0x1001) << 16) | (readData_2600(0x1002) << 8) | (readData_2600(0x1003));
  if (testdata == 0xFFFFFFFF)
    return true;
  else
    return false;
}

void readROM_2600() {
  strcpy(fileName, romName);
  strcat(fileName, ".a26");

  // create a new folder for storing rom file
  EEPROM_readAnything(FOLDER_NUM, foldern);
  sprintf_P(folder, PSTR("ATARI/ROM/%d"), foldern);
  sd.mkdir(folder, true);
  sd.chdir(folder);

  display_Clear();
  print_Msg(F("Saving to "));
  print_Msg(folder);
  println_Msg(F("/..."));
  display_Update();

  // open file on sdcard
  if (!myFile.open(fileName, O_RDWR | O_CREAT))
    print_FatalError(create_file_STR);

  // write new folder number back to EEPROM
  foldern++;
  EEPROM_writeAnything(FOLDER_NUM, foldern);

  // ROM Start 0xF000
  // Address A12-A0 = 0x1000 = 1 0000 0000 0000 = 4KB
  // Read Start 0x1000

  switch (a2600mapper) {
    case 0x20:  // 2K Standard 2KB
      readSegment_2600(0x1000, 0x1800);
      break;

    case 0x3F:  // 3F Mapper 8KB
      for (int x = 0; x < 0x3; x++) {
        writeData3F_2600(0x3F, x);
        readSegment_2600(0x1000, 0x1800);
      }
      readSegment_2600(0x1800, 0x2000);
      break;

    case 0x40:  // 4K Default 4KB
      readSegment_2600(0x1000, 0x2000);
      break;

    case 0xC0:  // CV Mapper 2KB
      readSegment_2600(0x1800, 0x2000);
      break;

    case 0xD0:  // DPC Mapper 10KB
      // 8K ROM
      for (int x = 0; x < 0x2; x++) {
        readData_2600(0x1FF8 + x);
        // Split Read of 1st 0x200 bytes
        // 0x0000-0x0080 are DPC Registers (Random on boot)
        for (int y = 0; y < 0x80; y++) {
          sdBuffer[y] = 0xFF;  // Output 0xFFs for Registers
        }
        myFile.write(sdBuffer, 128);
        for (int z = 0; z < 0x180; z++) {
          sdBuffer[z] = readData_2600(0x1080 + z);
        }
        myFile.write(sdBuffer, 384);
        // Read Segment
        readSegment_2600(0x1200, 0x1800);
        // 0x1000-0x1080 are DPC Registers (Random on boot)
        for (int y = 0; y < 0x80; y++) {
          sdBuffer[y] = 0xFF;  // Output 0xFFs for Registers
        }
        myFile.write(sdBuffer, 128);
        for (int z = 0; z < 0x180; z++) {
          sdBuffer[z] = readData_2600(0x1880 + z);
        }
        myFile.write(sdBuffer, 384);
        // Read Segment
        readSegment_2600(0x1A00, 0x1E00);
        // Split Read of Last 0x200 bytes
        for (int y = 0; y < 0x1F8; y++) {
          sdBuffer[y] = readData_2600(0x1E00 + y);
        }
        myFile.write(sdBuffer, 504);
        for (int z = 0; z < 8; z++) {
          // Set Bank to ensure 0x1FFA-0x1FFF is correct
          readData_2600(0x1FF8 + x);
          sdBuffer[z] = readData_2600(0x1FF8 + z);
        }
        myFile.write(sdBuffer, 8);
      }

      // 2K DPC Internal Graphics ROM
      // Read Registers 0x1008-0x100F (Graphics 0x1008-0x100C)
      // Write Registers LSB 0x1050-0x1057 AND MSB 0x1058-0x105F

      // Set Data Fetcher 0 Limits
      writeData_2600(0x1040, 0xFF);  // MAX for Data Fetcher 0
      writeData_2600(0x1048, 0x00);  // MIN for Data Fetcher 0
      // Set Data Fetcher 0 Counter (0x7FF)
      writeData_2600(0x1050, 0xFF);  // LSB for Data Fetcher 0
      writeData_2600(0x1058, 0x07);  // MSB for Data Fetcher 0
      // Set Data Fetcher 1 Counter (0x7FF)
      writeData_2600(0x1051, 0xFF);  // LSB for Data Fetcher 1
      writeData_2600(0x1059, 0x07);  // MSB for Data Fetcher 1
      for (int x = 0; x < 0x800; x += 512) {
        for (int y = 0; y < 512; y++) {
          sdBuffer[y] = readData_2600(0x1008);  // Data Fetcher 0
          readData_2600(0x1009);                // Data Fetcher 1
        }
        myFile.write(sdBuffer, 512);
      }
      break;

    case 0xE0:  // E0 Mapper 8KB
      for (int x = 0; x < 0x7; x++) {
        readData_2600(0x1FE0 + x);
        readSegment_2600(0x1000, 0x1400);
      }
      readSegment_2600(0x1C00, 0x2000);
      break;

    case 0xE7: // E7 Mapper 8KB/12KB/16KB
      // Check Bank 0 - If 0xFFs then Bump 'n' Jump
      if (checkE7(0)) { // Bump 'n' Jump 8K
        writeData_2600(0x1800, 0xFF);

        for (int x = 4; x < 7; x++) { // Banks 4-6
          readData_2600(0x1FE0 + x);
          readSegment_2600(0x1000, 0x1800);
        }
        e7size = 0;
      }
      // Check Bank 3 - If 0xFFs then BurgerTime
      else if (checkE7(3)) { // BurgerTime 12K
        writeData_2600(0x1800, 0xFF);
        for (int x = 0; x < 2; x++) { // Banks 0+1
          readData_2600(0x1FE0 + x);
          readSegment_2600(0x1000, 0x1800);
        }
        for (int x = 4; x < 7; x++) { // Banks 4-6
          readData_2600(0x1FE0 + x);
          readSegment_2600(0x1000, 0x1800);
        }
        e7size = 1;
      }
      else { // Masters of the Universe (or Unknown Cart) 16K
        writeData_2600(0x1800, 0xFF);
        for (int x = 0; x < 7; x++) { // Banks 0-6
          readData_2600(0x1FE0 + x);
          readSegment_2600(0x1000, 0x1800);
        }
        e7size = 2;
      }
      readSegment_2600(0x1800, 0x2000); // Bank 7
      break;

    case 0xF0:  // F0 Mapper 64KB
      for (int x = 0; x < 0x10; x++) {
        readData_2600(0x1FF0);
        readSegment_2600(0x1000, 0x2000);
      }
      break;

    case 0xF4: // F4 Mapper 32KB
      for (int x = 0; x < 8; x++) {
        readData_2600(0x1FF4 + x);
        readSegment_2600(0x1000, 0x1E00);
        // Split Read of Last 0x200 bytes
        for (int y = 0; y < 0x1F4; y++) {
          sdBuffer[y] = readData_2600(0x1E00 + y);
        }
        myFile.write(sdBuffer, 500);
        for (int z = 0; z < 12; z++) {
          // Set Bank to ensure 0x1FFC-0x1FFF is correct
          readData_2600(0x1FF4 + x);
          sdBuffer[z] = readData_2600(0x1FF4 + z);
        }
        myFile.write(sdBuffer, 12);
      }
      break;

    case 0xF6: // F6 Mapper 16KB
      for (int w = 0; w < 4; w++) {
        readData_2600(0x1FF6 + w);
        readSegment_2600(0x1000, 0x1E00);
        // Split Read of Last 0x200 bytes
        for (int x = 0; x < 0x1F6; x++) {
          sdBuffer[x] = readData_2600(0x1E00 + x);
        }
        myFile.write(sdBuffer, 502);
        // Bank Registers 0x1FF6-0x1FF9
        for (int y = 0; y < 4; y++){
          readData_2600(0x1FFF); // Reset Bank
          sdBuffer[y] = readData_2600(0x1FF6 + y);
        }
        // End of Bank 0x1FFA-0x1FFF
        readData_2600(0x1FFF); // Reset Bank
        readData_2600(0x1FF6 + w); // Set Bank
        for (int z = 4; z < 10; z++) {
          sdBuffer[z] = readData_2600(0x1FF6 + z); // 0x1FFA-0x1FFF
        }
        myFile.write(sdBuffer, 10);
      }
      readData_2600(0x1FFF); // Reset Bank
      break;

    case 0xF8: // F8 Mapper 8KB
      for (int w = 0; w < 2; w++) {
        readData_2600(0x1FF8 + w);
        readSegment_2600(0x1000, 0x1E00);
        // Split Read of Last 0x200 bytes
        for (int x = 0; x < 0x1F8; x++) {
          sdBuffer[x] = readData_2600(0x1E00 + x);
        }
        myFile.write(sdBuffer, 504);
        // Bank Registers 0x1FF8-0x1FF9
        for (int y = 0; y < 2; y++){
          readData_2600(0x1FFF); // Reset Bank
          sdBuffer[y] = readData_2600(0x1FF8 + y);
        }
        // End of Bank 0x1FFA-0x1FFF
        readData_2600(0x1FFF); // Reset Bank
        readData_2600(0x1FF8 + w); // Set Bank
        for (int z = 2; z < 8; z++) {
          sdBuffer[z] = readData_2600(0x1FF8 + z); // 0x1FFA-0x1FFF
        }
        myFile.write(sdBuffer, 8);
      }
      readData_2600(0x1FFF); // Reset Bank
      break;

    case 0xF9: // Time Pilot Mapper 8KB
      // Bad implementation of the F8 Mapper
      // kevtris swapped the bank order - swapped banks may not match physical ROM data
      // Bankswitch code uses 0x1FFC and 0x1FF9
      for (int w = 3; w >= 0; w -= 3) {
        readData_2600(0x1FF9 + w);
        readSegment_2600(0x1000, 0x1E00);
        // Split Read of Last 0x200 bytes
        for (int x = 0; x < 0x1F9; x++) {
          sdBuffer[x] = readData_2600(0x1E00 + x);
        }
        myFile.write(sdBuffer, 505);
        readData_2600(0x1FFF); // Reset Bank
        sdBuffer[0] = readData_2600(0x1FF9);
        // End of Bank 0x1FFA-0x1FFF
        readData_2600(0x1FFF); // Reset Bank
        readData_2600(0x1FF9 + w); // Set Bank
        for (int z = 1; z < 7; z++) {
          sdBuffer[z] = readData_2600(0x1FF9 + z); // 0x1FFA-0x1FFF
        }
        myFile.write(sdBuffer, 7);
      }
      // Reset Bank
      readData_2600(0x1FF9);
      readData_2600(0x1FFF);
      readData_2600(0x1FFC);
      break;

    case 0xFA:  // FA Mapper 12KB
      for (int x = 0; x < 0x3; x++) {
        writeData_2600(0x1FF8 + x, 0x1);  // Set Bank with D0 HIGH
        readSegment_2600(0x1000, 0x1E00);
        // Split Read of Last 0x200 bytes
        for (int y = 0; y < 0x1F8; y++) {
          sdBuffer[y] = readData_2600(0x1E00 + y);
        }
        myFile.write(sdBuffer, 504);
        for (int z = 0; z < 8; z++) {
          // Set Bank to ensure 0x1FFB-0x1FFF is correct
          writeData_2600(0x1FF8 + x, 0x1);  // Set Bank with D0 HIGH
          sdBuffer[z] = readData_2600(0x1FF8 + z);
        }
        myFile.write(sdBuffer, 8);
      }
      break;

    case 0xFE:  // FE Mapper 8KB
      for (int x = 0; x < 0x2; x++) {
        writeData_2600(0x01FE, 0xF0 ^ (x << 5));
        writeData_2600(0x01FF, 0xF0 ^ (x << 5));
        readSegment_2600(0x1000, 0x2000);
      }
      break;

    case 0x0A:  // UA Mapper 8KB
      readData_2600(0x220);
      readSegment_2600(0x1000, 0x2000);
      readData_2600(0x240);
      readSegment_2600(0x1000, 0x2000);
      break;
  }
  myFile.close();

  unsigned long crcsize = a2600[a2600size] * 0x400;
  // Correct E7 Size for 8K/12K ROMs
  if (a2600mapper == 0xE7) {
    if (e7size == 0)
      crcsize = a2600[a2600size] * 0x200;
    else if (e7size == 1)
      crcsize = a2600[a2600size] * 0x300;
  }
  calcCRC(fileName, crcsize, NULL, 0);

  println_Msg(F(""));
  print_STR(press_button_STR, 1);
  display_Update();
  wait();
}

//******************************************
// ROM SIZE
//******************************************

void checkStatus_2600() {
  EEPROM_readAnything(ATARI2600_MAPPER, a2600mapper);
  EEPROM_readAnything(ATARI2600_ROM_SIZE, a2600size);
  if (a2600size > 6) {
    a2600size = 1;  // default 4KB
    EEPROM_writeAnything(ATARI2600_ROM_SIZE, a2600size);
  }

#if (defined(enable_OLED) || defined(enable_LCD))
  display_Clear();
  println_Msg(F("ATARI 2600 READER"));
  println_Msg(F("CURRENT SETTINGS"));
  println_Msg(F(""));
  print_Msg(F("MAPPER:   "));
  if (a2600mapper == 0x20)
    println_Msg(F("2K"));
  else if (a2600mapper == 0x40)
    println_Msg(F("4K"));
  else if (a2600mapper == 0x0A)
    println_Msg(F("UA"));
  else if (a2600mapper == 0xC0)
    println_Msg(F("CV"));
  else if (a2600mapper == 0xD0)
    println_Msg(F("DPC"));
  else if (a2600mapper == 0xF9)
    println_Msg(F("TP"));
  else
    println_Msg(a2600mapper, HEX);
  print_Msg(F("ROM SIZE: "));
  if (a2600mapper == 0xD0)
    print_Msg(F("10"));
  else
    print_Msg(a2600[a2600size]);
  println_Msg(F("K"));
  display_Update();
  wait();
#else
  Serial.print(F("MAPPER:   "));
  if (a2600mapper == 0x20)
    Serial.println(F("2K"));
  else if (a2600mapper == 0x40)
    Serial.println(F("4K"));
  else if (a2600mapper == 0x0A)
    Serial.println(F("UA"));
  else if (a2600mapper == 0xC0)
    Serial.println(F("CV"));
  else if (a2600mapper == 0xD0)
    Serial.println(F("DPC"));
  else if (a2600mapper == 0xF9)
    Serial.println(F("TP"));
  else
    Serial.println(a2600mapper, HEX);
  Serial.print(F("ROM SIZE: "));
  if (a2600mapper == 0xD0)
    Serial.print(F("10"));
  else
    Serial.print(a2600[a2600size]);
  Serial.println(F("K"));
  Serial.println(F(""));
#endif
}

//******************************************
// SET MAPPER
//******************************************

void setMapper_2600() {
#if (defined(enable_OLED) || defined(enable_LCD))
  int b = 0;
  int i = 0;
  // Check Button Status
#if defined(enable_OLED)
  buttonVal1 = (PIND & (1 << 7));  // PD7
#elif defined(enable_LCD)
  boolean buttonVal1 = (PING & (1 << 2));  //PG2
#endif

  if (buttonVal1 == LOW) {  // Button Pressed
    while (1) {             // Scroll Mapper List
#if defined(enable_OLED)
      buttonVal1 = (PIND & (1 << 7));  // PD7
#elif defined(enable_LCD)
      buttonVal1 = (PING & (1 << 2));      //PG2
#endif
      if (buttonVal1 == HIGH) {  // Button Released
        // Correct Overshoot
        if (i == 0)
          i = a2600mapcount - 1;
        else
          i--;
        break;
      }
      display_Clear();
      print_Msg(F("Mapper: "));
      a2600index = i * 2;
      a2600mapselect = pgm_read_byte(a2600mapsize + a2600index);
      if (a2600mapselect == 0x20)
        println_Msg(F("2K"));
      else if (a2600mapselect == 0x40)
        println_Msg(F("4K"));
      else if (a2600mapselect == 0x0A)
        println_Msg(F("UA"));
      else if (a2600mapselect == 0xC0)
        println_Msg(F("CV"));
      else if (a2600mapselect == 0xD0)
        println_Msg(F("DPC"));
      else if (a2600mapselect == 0xF9)
        println_Msg(F("TP"));
      else
        println_Msg(a2600mapselect, HEX);
      display_Update();
      if (i == (a2600mapcount - 1))
        i = 0;
      else
        i++;
      delay(250);
    }
  }

  display_Clear();
  print_Msg(F("Mapper: "));
  a2600index = i * 2;
  a2600mapselect = pgm_read_byte(a2600mapsize + a2600index);
  if (a2600mapselect == 0x20)
    println_Msg(F("2K"));
  else if (a2600mapselect == 0x40)
    println_Msg(F("4K"));
  else if (a2600mapselect == 0x0A)
    println_Msg(F("UA"));
  else if (a2600mapselect == 0xC0)
    println_Msg(F("CV"));
  else if (a2600mapselect == 0xD0)
    println_Msg(F("DPC"));
  else if (a2600mapselect == 0xF9)
    println_Msg(F("TP"));
  else
    println_Msg(a2600mapselect, HEX);
  println_Msg(F(""));
#if defined(enable_OLED)
  print_STR(press_to_change_STR, 1);
  print_STR(right_to_select_STR, 1);
#elif defined(enable_LCD)
  print_STR(rotate_to_change_STR, 1);
  print_STR(press_to_select_STR, 1);
#endif
  display_Update();

  while (1) {
    b = checkButton();
    if (b == 2) {  // Previous Mapper (doubleclick)
      if (i == 0)
        i = a2600mapcount - 1;
      else
        i--;

      // Only update display after input because of slow LCD library
      display_Clear();
      print_Msg(F("Mapper: "));
      a2600index = i * 2;
      a2600mapselect = pgm_read_byte(a2600mapsize + a2600index);
      if (a2600mapselect == 0x20)
        println_Msg(F("2K"));
      else if (a2600mapselect == 0x40)
        println_Msg(F("4K"));
      else if (a2600mapselect == 0x0A)
        println_Msg(F("UA"));
      else if (a2600mapselect == 0xC0)
        println_Msg(F("CV"));
      else if (a2600mapselect == 0xD0)
        println_Msg(F("DPC"));
      else if (a2600mapselect == 0xF9)
        println_Msg(F("TP"));
      else
        println_Msg(a2600mapselect, HEX);
      println_Msg(F(""));
#if defined(enable_OLED)
      print_STR(press_to_change_STR, 1);
      print_STR(right_to_select_STR, 1);
#elif defined(enable_LCD)
      print_STR(rotate_to_change_STR, 1);
      print_STR(press_to_select_STR, 1);
#endif
      display_Update();
    }
    if (b == 1) {  // Next Mapper (press)
      if (i == (a2600mapcount - 1))
        i = 0;
      else
        i++;

      // Only update display after input because of slow LCD library
      display_Clear();
      print_Msg(F("Mapper: "));
      a2600index = i * 2;
      a2600mapselect = pgm_read_byte(a2600mapsize + a2600index);
      if (a2600mapselect == 0x20)
        println_Msg(F("2K"));
      else if (a2600mapselect == 0x40)
        println_Msg(F("4K"));
      else if (a2600mapselect == 0x0A)
        println_Msg(F("UA"));
      else if (a2600mapselect == 0xC0)
        println_Msg(F("CV"));
      else if (a2600mapselect == 0xD0)
        println_Msg(F("DPC"));
      else if (a2600mapselect == 0xF9)
        println_Msg(F("TP"));
      else
        println_Msg(a2600mapselect, HEX);
      println_Msg(F(""));
#if defined(enable_OLED)
      print_STR(press_to_change_STR, 1);
      print_STR(right_to_select_STR, 1);
#elif defined(enable_LCD)
      print_STR(rotate_to_change_STR, 1);
      print_STR(press_to_select_STR, 1);
#endif
      display_Update();
    }
    if (b == 3) {  // Long Press - Execute (hold)
      new2600mapper = a2600mapselect;
      break;
    }
  }
  display.setCursor(0, 56);
  print_Msg(F("MAPPER "));
  if (new2600mapper == 0x20)
    println_Msg(F("2K"));
  else if (new2600mapper == 0x40)
    println_Msg(F("4K"));
  if (new2600mapper == 0x0A)
    print_Msg(F("UA"));
  else if (new2600mapper == 0xC0)
    print_Msg(F("CV"));
  else if (new2600mapper == 0xD0)
    println_Msg(F("DPC"));
  else if (new2600mapper == 0xF9)
    println_Msg(F("TP"));
  else
    print_Msg(new2600mapper, HEX);
  println_Msg(F(" SELECTED"));
  display_Update();
  delay(1000);
#else
setmapper:
  String newmap;
  Serial.println(F("SUPPORTED MAPPERS:"));
  Serial.println(F("0 = 2K [Standard 2K]"));
  Serial.println(F("1 = 3F [Tigervision]"));
  Serial.println(F("2 = 4K [Standard 4K]"));
  Serial.println(F("3 = CV [Commavid]"));
  Serial.println(F("4 = DPC [Pitfall II]"));
  Serial.println(F("5 = E0 [Parker Bros]"));
  Serial.println(F("6 = E7 [M-Network]"));
  Serial.println(F("7 = F0 [Megaboy]"));
  Serial.println(F("8 = F4 [Atari 32K]"));
  Serial.println(F("9 = F6 [Atari 16K]"));
  Serial.println(F("10 = F8 [Atari 8K]"));
  Serial.println(F("11 = FA [CBS RAM Plus]"));
  Serial.println(F("12 = FE [Activision]"));
  Serial.println(F("13 = TP [Time Pilot 8K]"));
  Serial.println(F("14 = UA [UA Ltd]"));
  Serial.print(F("Enter Mapper [0-14]: "));
  while (Serial.available() == 0) {}
  newmap = Serial.readStringUntil('\n');
  Serial.println(newmap);
  a2600index = newmap.toInt() * 2;
  new2600mapper = pgm_read_byte(a2600mapsize + a2600index);
#endif
  EEPROM_writeAnything(ATARI2600_MAPPER, new2600mapper);
  a2600mapper = new2600mapper;

  a2600size = pgm_read_byte(a2600mapsize + a2600index + 1);
  EEPROM_writeAnything(ATARI2600_ROM_SIZE, a2600size);
}

//******************************************
// CART SELECT CODE
//******************************************

FsFile a2600csvFile;
char a2600game[36];                     // title
char a2600mm[4];                        // mapper
char a2600ll[4];                        // linelength (previous line)
unsigned long a2600csvpos;              // CSV File Position
char a2600cartCSV[] = "2600.txt";       // CSV List
char a2600csvEND[] = "EOF";             // CSV End Marker for scrolling

bool readLine_ATARI(FsFile& f, char* line, size_t maxLen) {
  for (size_t n = 0; n < maxLen; n++) {
    int c = f.read();
    if (c < 0 && n == 0) return false;  // EOF
    if (c < 0 || c == '\n') {
      line[n] = 0;
      return true;
    }
    line[n] = c;
  }
  return false;  // line too long
}

bool readVals_ATARI(char* a2600game, char* a2600mm, char* a2600ll) {
  char line[42];
  a2600csvpos = a2600csvFile.position();
  if (!readLine_ATARI(a2600csvFile, line, sizeof(line))) {
    return false;  // EOF or too long
  }
  char* comma = strtok(line, ",");
  int x = 0;
  while (comma != NULL) {
    if (x == 0)
      strcpy(a2600game, comma);
    else if (x == 1)
      strcpy(a2600mm, comma);
    else if (x == 2)
      strcpy(a2600ll, comma);
    comma = strtok(NULL, ",");
    x += 1;
  }
  return true;
}

bool getCartListInfo_2600() {
  bool buttonreleased = 0;
  bool cartselected = 0;
#if (defined(enable_OLED) || defined(enable_LCD))
  display_Clear();
  println_Msg(F(" HOLD TO FAST CYCLE"));
  display_Update();
#else
  Serial.println(F("HOLD BUTTON TO FAST CYCLE"));
#endif
  delay(2000);
#if defined(enable_OLED)
  buttonVal1 = (PIND & (1 << 7));  // PD7
#elif defined(enable_LCD)
  boolean buttonVal1 = (PING & (1 << 2));  //PG2
#endif
  if (buttonVal1 == LOW) {  // Button Held - Fast Cycle
    while (1) {             // Scroll Game List
      while (readVals_ATARI(a2600game, a2600mm, a2600ll)) {
        if (strcmp(a2600csvEND, a2600game) == 0) {
          a2600csvFile.seek(0);  // Restart
        } else {
#if (defined(enable_OLED) || defined(enable_LCD))
          display_Clear();
          println_Msg(F("CART TITLE:"));
          println_Msg(F(""));
          println_Msg(a2600game);
          display_Update();
#else
          Serial.print(F("CART TITLE:"));
          Serial.println(a2600game);
#endif
#if defined(enable_OLED)
          buttonVal1 = (PIND & (1 << 7));  // PD7
#elif defined(enable_LCD)
          buttonVal1 = (PING & (1 << 2));  //PG2
#endif
          if (buttonVal1 == HIGH) {  // Button Released
            buttonreleased = 1;
            break;
          }
          if (buttonreleased) {
            buttonreleased = 0;  // Reset Flag
            break;
          }
        }
      }
#if defined(enable_OLED)
      buttonVal1 = (PIND & (1 << 7));  // PD7
#elif defined(enable_LCD)
      buttonVal1 = (PING & (1 << 2));      //PG2
#endif
      if (buttonVal1 == HIGH)  // Button Released
        break;
    }
  }
#if (defined(enable_OLED) || defined(enable_LCD))
  display.setCursor(0, 56);
  println_Msg(F("FAST CYCLE OFF"));
  display_Update();
#else
  Serial.println(F(""));
  Serial.println(F("FAST CYCLE OFF"));
  Serial.println(F("PRESS BUTTON TO STEP FORWARD"));
  Serial.println(F("DOUBLE CLICK TO STEP BACK"));
  Serial.println(F("HOLD TO SELECT"));
  Serial.println(F(""));
#endif
  while (readVals_ATARI(a2600game, a2600mm, a2600ll)) {
    if (strcmp(a2600csvEND, a2600game) == 0) {
      a2600csvFile.seek(0);  // Restart
    } else {
#if (defined(enable_OLED) || defined(enable_LCD))
      display_Clear();
      println_Msg(F("CART TITLE:"));
      println_Msg(F(""));
      println_Msg(a2600game);
      display.setCursor(0, 48);
#if defined(enable_OLED)
      print_STR(press_to_change_STR, 1);
      print_STR(right_to_select_STR, 1);
#elif defined(enable_LCD)
      print_STR(rotate_to_change_STR, 1);
      print_STR(press_to_select_STR, 1);
#endif
      display_Update();
#else
      Serial.print(F("CART TITLE:"));
      Serial.println(a2600game);
#endif
      while (1) {  // Single Step
        int b = checkButton();
        if (b == 1) {  // Continue (press)
          break;
        }
        if (b == 2) {  // Reset to Start of List (doubleclick)
          byte prevline = strtol(a2600ll, NULL, 10);
          a2600csvpos -= prevline;
          a2600csvFile.seek(a2600csvpos);
          break;
        }
        if (b == 3) {  // Long Press - Select Cart (hold)
          new2600mapper = strtol(a2600mm, NULL, 10);
          EEPROM_writeAnything(ATARI2600_MAPPER, new2600mapper);
          cartselected = 1;  // SELECTION MADE
#if (defined(enable_OLED) || defined(enable_LCD))
          println_Msg(F("SELECTION MADE"));
          display_Update();
#else
          Serial.println(F("SELECTION MADE"));
#endif
          break;
        }
      }
      if (cartselected) {
        cartselected = 0;  // Reset Flag
        return true;
      }
    }
  }
#if (defined(enable_OLED) || defined(enable_LCD))
  println_Msg(F(""));
  println_Msg(F("END OF FILE"));
  display_Update();
#else
  Serial.println(F("END OF FILE"));
#endif

  return false;
}

void checkCSV_2600() {
  if (getCartListInfo_2600()) {
#if (defined(enable_OLED) || defined(enable_LCD))
    display_Clear();
    println_Msg(F("CART SELECTED"));
    println_Msg(F(""));
    println_Msg(a2600game);
    display_Update();
    // Display Settings
    display.setCursor(0, 56);
    print_Msg(F("CODE: "));
    println_Msg(new2600mapper, HEX);
    display_Update();
#else
    Serial.println(F(""));
    Serial.println(F("CART SELECTED"));
    Serial.println(a2600game);
    // Display Settings
    Serial.print(F("CODE: "));
    Serial.println(new2600mapper, HEX);
    Serial.println(F(""));
#endif
  } else {
#if (defined(enable_OLED) || defined(enable_LCD))
    display.setCursor(0, 56);
    println_Msg(F("NO SELECTION"));
    display_Update();
#else
    Serial.println(F("NO SELECTION"));
#endif
  }
}

void checkSize_2600() {
  EEPROM_readAnything(ATARI2600_MAPPER, a2600mapper);
  for (int i = 0; i < a2600mapcount; i++) {
    a2600index = i * 2;
    if (a2600mapper == pgm_read_byte(a2600mapsize + a2600index)) {
      a2600size = pgm_read_byte(a2600mapsize + a2600index + 1);
      EEPROM_writeAnything(ATARI2600_ROM_SIZE, a2600size);
      break;
    }
  }
}

void setCart_2600() {
#if (defined(enable_OLED) || defined(enable_LCD))
  display_Clear();
  println_Msg(a2600cartCSV);
  display_Update();
#endif
  sd.chdir();
  sprintf_P(folder, PSTR("2600/CSV"));
  sd.chdir(folder);  // Switch Folder
  a2600csvFile = sd.open(a2600cartCSV, O_READ);
  if (!a2600csvFile) {
#if (defined(enable_OLED) || defined(enable_LCD))
    display_Clear();
    println_Msg(F("CSV FILE NOT FOUND!"));
    display_Update();
#else
    Serial.println(F("CSV FILE NOT FOUND!"));
#endif
    while (1) {
      if (checkButton() != 0)
        setup_2600();
    }
  }
  checkCSV_2600();
  a2600csvFile.close();

  checkSize_2600();
}
#endif
//******************************************
// End of File
//******************************************