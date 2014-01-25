typedef struct {
  uint8_t segaboot[0xA0000]; //TODO: Union with xbeHeader etc. The xbe header seems to be broken though, it also uses custom keys (however, for games the xbox keys are used again)
  uint8_t resources[0x44000]; //mbrom files(?)
  uint8_t firmwareAsic[0x18000]; // *not entirely sure where it starts* Firmware.asic for
                                 // *probably* an Renesas (previously NEC) RX850 OR vxWorks.
                                 // Better get someone with a real chihiro to doublecheck.
                                 // (the key for this file is 0xEECCAA8866442200 [or bytewise reversed, depending on the interpretation])
                                 // (- Seems to be called marble_firm.bin in virtua cop 3!)
  uint8_t firmwareBin[0x2000]; // Maybe still part of the firmware.asic file, but even in the first page this seems to have undecrypted patterns
  uint8_t firmware2Bin[0x1E00]; // Contains the string SEGA BASEBD (unicode or something though, but looks like it isn't encrypted)
  uint8_t mbdt[0x200];
} firmware_t;

typedef struct {
  firmware_t old;
  firmware_t new;
} firmwareRom_t;
