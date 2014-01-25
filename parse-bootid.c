#include <stdio.h>
#include <stdint.h>
#include <string.h>


typedef struct {
  char magic[4];
  uint8_t unk7[0xC];
  //32 bit: 480 (something to do with resolution?)
  //32 bit: 1
  //32 bit: 1
  uint8_t pad[0x10];

  char xbam[4]; //Xbox arcade [/amusement?] machine?
  uint8_t unk6[4];
  uint16_t unkYear; //0x28: 16 bit: year of xbamgd.bin release? 2002
  uint8_t unkDay;
  uint8_t unkMonth;
  uint8_t unk8[4]; //xbam version?

  char gameId[8]; //0x30: GameID
  uint8_t unk9[0x28];
  char maker[0x20]; //0x60: maker?
  char title[0x20]; //0x80: title
  char gameXbe[0x20]; //0xA0: game xbe
  char testXbe[0x20]; //0xC0: test xbe
  // The following ones are either zero terminated OR the first symbol is checked for 0x00
  char startCredits[0x20]; //0xE0: test mode option 1? Maybe DIPSW settings?
  char continueCredits[0x20]; //0x100: test mode option 2?
  char unk0[0x20]; //0x120: test mode option 3?
  char unk1[0x20]; //0x140: test mode option 4?
  char unk2[0x20]; //0x160: test mode option 5?
  char unk3[0x20]; //0x180: test mode option 6?
  char unk4[0x20]; //0x1A0: test mode option 7?
  char unk5[0x20]; //0x1C0: test mode option 8?
} __attribute__((packed)) bootId_t;

void compareAndDumpData(const char* mode, const char* name, uint8_t* data, size_t length) {
  if (!strcmp(mode,name)) {
    int i;
    for(i = 0; i < length; i++) {
      printf("%02X",data[i]);
    }
    printf("\n");
  }
}

void compareAndDumpString(const char* mode, const char* name, const char* string, size_t length) {
  if (!strcmp(mode,name)) {
    int i;
    for(i = 0; i < length; i++) {
      if (string[i] == 0x00) {
        printf("\t");
      } else {
        printf("%c",string[i]); 
      }
    }
    printf("\n");
  }
}

int main(int argc, char* argv[]) {

  bootId_t bid;

  char* source = argv[1];
  char* mode = argv[2];
  
  FILE* bin = fopen(source,"rb");   
  fseek(bin,0,SEEK_END);  
  size_t length = ftell(bin);
  if (length != sizeof(bootId_t)) {
    printf("File wasn't of correct size - maybe not a boot.id?\n");
    fclose(bin);
    return 1;
  }
  fseek(bin,0,SEEK_SET);  
  fread(&bid,1,length,bin);
  fclose(bin);
  
  compareAndDumpString(mode,"magic",bid.magic,4);
  compareAndDumpData(mode,"unk7",bid.unk7,0xC);
  //TODO: pad could be dumped too?
  compareAndDumpString(mode,"xbam",bid.xbam,4);
  compareAndDumpData(mode,"unk6",bid.unk6,0x4);
  if (!strcmp(mode,"date")) {
    printf("%02i/%02i/%04i",bid.unkMonth,bid.unkDay,bid.unkYear);
  }
  compareAndDumpData(mode,"unk8",bid.unk8,0x4);
  compareAndDumpString(mode,"id",bid.gameId,8);
  compareAndDumpString(mode,"maker",bid.maker,0x20);
  compareAndDumpData(mode,"unk9",bid.unk9,0x28);
  compareAndDumpString(mode,"title",bid.title,0x20);
  compareAndDumpString(mode,"game-xbe",bid.gameXbe,0x20);
  compareAndDumpString(mode,"test-xbe",bid.testXbe,0x20);
  compareAndDumpString(mode,"start-credits",bid.startCredits,0x20);
  compareAndDumpString(mode,"continue-credits",bid.continueCredits,0x20);
  compareAndDumpString(mode,"unk0",bid.unk0,0x20);
  compareAndDumpString(mode,"unk1",bid.unk1,0x20);
  compareAndDumpString(mode,"unk2",bid.unk2,0x20);
  compareAndDumpString(mode,"unk3",bid.unk3,0x20);
  compareAndDumpString(mode,"unk4",bid.unk4,0x20);
  compareAndDumpString(mode,"unk5",bid.unk5,0x20);

  return 0;
}
