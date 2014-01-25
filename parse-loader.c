#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint8_t unk1[8];
  uint8_t unk2[8]; //0x08: Looks like a DES key to something, maybe a checksum?
  uint8_t unk3[0x90];
  uint8_t unk4[0x20]; //0xa0: mount dir of something (mediaboard? path to the fatx image to load?)
  uint8_t fatx[0x20]; //0xc0: name of the fatx image to load (1024 MB only?)
  uint8_t unk5[0x20]; //0xe0: name of the fatx image to load ? might be part of 0xc0
} __attribute__((packed)) loader_t;

int main(int argc, char* argv[]) {

  loader_t loader;

  char* source = argv[1];
  char* mode = argv[2];
  
  FILE* bin = fopen(source,"rb");   
  fseek(bin,0,SEEK_END);  
  size_t length = ftell(bin);
  if (length != sizeof(loader_t)) {
    printf("File wasn't of correct size - maybe not a loader bin?\n");
    fclose(bin);
    return 1;
  }
  fseek(bin,0,SEEK_SET);  
  fread(&loader,1,length,bin);
  fclose(bin);
  
  if (!strcmp(mode,"unk1")) {
    int i;
    for(i = 0; i < 8; i++) {
      printf("%02X",loader.unk1[i]);
    }
    printf("\n");
  }

  if (!strcmp(mode,"unk2")) {
    int i;
    for(i = 0; i < 8; i++) {
      printf("%02X",loader.unk2[i]);
    }
    printf("\n");
  }

  if (!strcmp(mode,"unk3")) {
    int i;
    for(i = 0; i < 0x90; i++) {
      printf("%02X",loader.unk3[i]);
    }
    printf("\n");
  }

  if (!strcmp(mode,"unk4")) { 
    printf("%s\n",loader.unk4);
  }

  if (!strcmp(mode,"fatx")) { 
    printf("%s\n",loader.fatx);
  }

  if (!strcmp(mode,"unk5")) { 
    printf("%s\n",loader.unk4);
  }

  return 0;
}
