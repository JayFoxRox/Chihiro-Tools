#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>

#ifdef OPENSSL
#include <openssl/des.h>
const unsigned int chunk = 8;
#else
#include <rpc/des_crypt.h>
const unsigned int chunk = 0x1000;
#endif

void swap(uint8_t* a, uint8_t* b) {
  uint8_t buffer = *a;
  *a = *b;
  *b = buffer;
  return;
}

void swapBuffer(uint8_t* buffer, size_t length) {
  unsigned int i;
  unsigned int j;
  unsigned int percent;
  unsigned int displayed;
  for(i = 0; i < length / 8; i++) {
    for(j = 0; j < 4; j++) {
      swap(&buffer[i * 8 + j], &buffer[i * 8 + (7 - j)]);
    }
    percent = (int)((float)i/((float)((length/8)-1)/100.0f));
    if (percent != displayed) {
      printf("\rProcess: %u%%",percent);
      displayed = percent;
    }
  }
  printf("\n");
} 

int main(int argc, char* argv[]) {

  uint8_t* buffer;
  size_t length;

  // Parse arguments

  char* source = argv[1];
  char* destination = argv[2];
  char* keyString = argv[3];
  uint8_t key[8];
  unsigned int i;
  for(i = 0; i < 8; i++) {
    int buffer;
    sscanf(keyString,"%02X",&buffer);
    key[7-i] = buffer & 0xFF;
    keyString = &keyString[2];
  }
  printf("Got %02X%02X%02X%02X%02X%02X%02X%02X as key\n",key[7],key[6],key[5],key[4],key[3],key[2],key[1],key[0]);

  // Load the original file

  {
    FILE* bin = fopen(source,"rb");
    fseek(bin,0,SEEK_END);  
    length = ftell(bin);
    fseek(bin,0,SEEK_SET);  
    buffer = malloc(length);
    fread(buffer,1,length,bin);
    fclose(bin);  
    printf("Read %i bytes from file!\n",(int)length);
  }

  // Complain

  if ((length % 8) != 0) { 
    printf("Size of file is not multiple of 8!\n");
    return 1;
  }

  // Swap 

  printf("Swapping\n");
  swapBuffer(buffer,length);

  // Decrypt

#ifdef OPENSSL
  printf("Decrypting using OpenSSL\n");
#else
  printf("Decrypting\n");
#endif
  unsigned int queue = length;
  while(queue >= chunk) {

  
    void* p = (char*)&buffer[length - queue];
  
#ifdef OPENSSL
    des_key_schedule sched;
    int result = DES_set_key((const_DES_cblock*)key, &sched);
    DES_ecb_encrypt((const_DES_cblock*)p, (DES_cblock*)p, &sched, DES_DECRYPT);
#else
//    char* iv[8];
//    memset(iv,0x00,8);
//    int result = cbc_crypt((char*)key, (char*)p, chunk, DES_DECRYPT | DES_HW, iv);  
    int result = ecb_crypt((char*)key, (char*)p, chunk, DES_DECRYPT | DES_HW);  
    //printf("Decryption result is %i\n",result);
#endif
    queue -= chunk;
  }
#ifndef OPENSSL
  if (queue > 0) {
    int result = ecb_crypt((char*)key, (char*)&buffer[length - queue], queue, DES_DECRYPT | DES_HW);  
    printf("Final decryption result is %i\n",result);
  }
#endif

  // Swap back

  printf("Swapping back\n");
  swapBuffer(buffer,length);

  // Store the new file

  {
    FILE* bin = fopen(destination,"wb");
    fwrite(buffer,1,length,bin);
    fclose(bin);  
    free(buffer);
    printf("Wrote %i bytes to file!\n",(int)length);
  }

  printf("Done!\n");

  return 0;
}
