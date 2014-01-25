#include <stdio.h>
#include <stdint.h>
#include <string.h>

typedef struct {
  uint8_t binaryChallengeResponse[8];
  uint8_t challenge8Response[8];
  uint8_t challenge7Response[8];
  uint8_t challenge6Response[8];
  uint8_t challenge5Response[8];
  uint8_t challenge4Response[8];
  uint8_t challenge3Response[8];
  uint8_t challenge2Response[8];
  uint8_t challenge1Response[8];
  uint8_t challenge0Response[8];
} securityIcDump_t;

const char* binaryChallenge = "kaijyo!?";
const char* challenge8 = "bsec_ver";
const char* challenge7 = "atestpic";
const char* challenge6 = "D1strdf1";
const char* challenge5 = "C1strdf0";
const char* challenge4 = "Bkeycode";
const char* challenge3 = "AKEYCODE";
//NOTE: These need the session key to be appended!
const char* challenge2 = "#"; 
const char* challenge1 = "\""; //TODO: tmbinc used "“" on his blog
const char* challenge0 = "!"; 

void compareAndDump(const char* mode, const char* challenge, uint8_t* response) {
  if (!strcmp(mode,challenge)) {
    int i;
    for(i = 0; i < 8; i++) {
      printf("%02X",response[i]);
    }
    printf("\n");
  }
}

int main(int argc,char* argv[]) {

  securityIcDump_t dump;

  char* source = argv[1];
  char* mode = argv[2];
  
  FILE* bin = fopen(source,"rb");   
  fseek(bin,0,SEEK_END);  
  size_t length = ftell(bin);
  if (length != sizeof(securityIcDump_t)) {
    printf("This is not a dumped security IC.\n");
    printf("This might be the PIC Firmware. Parsing this is not possible yet using this tool\n");
    fclose(bin);
    return 1;
  }
  fseek(bin,0,SEEK_SET);  
  fread(&dump,1,length,bin);
  fclose(bin);
  
  compareAndDump(mode,challenge0,dump.challenge0Response);
  compareAndDump(mode,challenge1,dump.challenge1Response);
  compareAndDump(mode,challenge2,dump.challenge2Response);
  compareAndDump(mode,challenge3,dump.challenge3Response);
  compareAndDump(mode,challenge4,dump.challenge4Response);
  compareAndDump(mode,challenge5,dump.challenge5Response);
  compareAndDump(mode,challenge6,dump.challenge6Response);
  compareAndDump(mode,challenge7,dump.challenge7Response);

  if (!strcmp(mode,"dimmid0")) {
    printf("%.7s\n",&dump.challenge0Response[1]);
  }

  if (!strcmp(mode,"dimmid1")) {
    printf("%.7s\n",&dump.challenge1Response[1]);
  }

  if (!strcmp(mode,"dimmid2")) {
    printf("%.7s\n",&dump.challenge2Response[1]);
  }

  if (!strcmp(mode,"key")) {
    int i;
    for(i = 1; i < 8; i++) {
      printf("%02X",dump.challenge3Response[i]);
    }
    printf("%02X\n",dump.challenge4Response[1]);
  }

  if (!strcmp(mode,"loader")) {
    printf("%.7s\n",&dump.challenge5Response[1]);
  }

  if (!strcmp(mode,"test")) {
    printf("%.7s\n",&dump.challenge7Response[1]);
  }

  if (!strcmp(mode,"version")) {
    printf("%.7s\n",&dump.challenge8Response[1]);
  }

  return 0;
}
