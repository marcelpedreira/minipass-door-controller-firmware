#ifndef use_external_memory
   #define write_white_memory(address, data) write_eeprom(address, data)
   #define read_white_memory(address) read_eeprom(address)
   #define write_regist_memory(address, data) write_eeprom(address + 0x80, data)
   #define read_regist_memory(address, data) read_eeprom(address + 0x80)
#else
   #include "i2c_memory.c"
   #define write_white_memory(address, data) write_ext_eeprom(address, data, 0)
   #define read_white_memory(address) read_ext_eeprom(address, 0)
   #define write_regist_memory(address, data) write_eeprom(address, data)
   #define read_regist_memory(address, data) read_eeprom(address)
#endif





