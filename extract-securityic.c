// NOTE: Not an emulator - only extracts the data section.
//       So only works if this is similar to the Crazy Taxi firmware structure.

#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint8_t code[0x600];
  union {
    uint8_t data[0x3A00];  
    struct {
      uint16_t unk1;
      uint8_t challenges[0x38*2];
      uint16_t unk2;
      uint8_t response0[0x8*2]; //DIMMID0
      uint16_t unk3;
      uint8_t response1[0x8*2]; //DIMMID1
      uint16_t unk4;
      uint8_t response2[0x8*2]; //DIMMID2
      uint16_t unk5;
      uint8_t response7[0x8*2]; //TEST_OK
      uint16_t unk6;
      uint8_t response8[0x8*2]; //VER
      uint16_t unk7;
      uint8_t unk8[0x8*2]; // useless
      uint16_t unk9;
      uint8_t response9[0x8*2]; //:p.q
      uint8_t unk10[0x46*2];
      uint16_t unk11;
      uint8_t response3[0x8*2]; //3
      uint8_t pad1[0x7*2];
      uint16_t unk12;
      uint8_t response4[0x8*2]; //4
      uint8_t pad2[0x7*2];
      uint16_t unk13;
      uint8_t response5[0x8*2]; //5 loader
      uint8_t pad3[0x7*2];
      uint16_t unk14;
      uint8_t response6[0x8*2]; //6
    };
  };
} __attribute__((packed)) securityIc_t;

int main(int argc,char* argv[]) {

  securityIc_t pic;

  char* source = argv[1];
  char* destination = argv[2];
  
  {
    FILE* bin = fopen(source,"rb");   
    fseek(bin,0,SEEK_END);  
    size_t length = ftell(bin);
    if (length != sizeof(securityIc_t)) {
      printf("This is not a security IC.\n");
      printf("This might be the response file already. Try to parse it instead\n");
      fclose(bin);
      return 1;
    }
    fseek(bin,0,SEEK_SET);  
    fread(&pic,1,length,bin);
    fclose(bin);
  }

  uint8_t buffer[80];
  int i;
  for(i = 0; i < 8; i++) { buffer[8*0+i] = pic.response9[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*1+i] = pic.response8[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*2+i] = pic.response7[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*3+i] = pic.response6[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*4+i] = pic.response5[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*5+i] = pic.response4[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*6+i] = pic.response3[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*7+i] = pic.response2[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*8+i] = pic.response1[2*i]; }
  for(i = 0; i < 8; i++) { buffer[8*9+i] = pic.response0[2*i]; }

  {
    FILE* bin = fopen(destination,"wb");
    fwrite(buffer,1,80,bin);
    fclose(bin);  
  } 

  return 0;
}
