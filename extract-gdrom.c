#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>

// Chihiro stuff:

typedef struct {
  char hardwareId[0x10];
  char makerId[0x10];
  char deviceInformation[0x10];
  char areaSymbols[0x3];
  char unusedAreaSymbols[0x5];
  char periphals[0x8];
  char productNumber[0xA];
  char productVersion[0x6];
  char releaseDate[0x10];
  char bootFile[0x10]; //NOTE: Unused on chihiro?
  char producer[0x10];
  char title[0x80];
} __attribute__((packed)) metaInformation_t;

typedef struct {
  uint8_t unknown[0x200];
} __attribute__((packed)) tableOfContents_t;

typedef struct {
  uint8_t code[4];
  char area[28];
} __attribute__((packed)) areaProtection_t;

typedef struct {
  metaInformation_t metaInformation;
  tableOfContents_t tableOfContents;
  uint8_t segaLogo[0x3400]; //TODO: Does chihiro still use the SH4 katana / naomi / dreamcast stuff ?
  areaProtection_t areaProtections[8];  
  uint8_t bootstrap1[0x2800]; 
  uint8_t bootstrap2[0x2000];
} __attribute__((packed)) gdrom_t;

char* areaSymbols[] = { "J", "U", "E" };
char* areaProtections[] = { "For JAPAN,TAIWAN,PHILIPINES.", "For USA and CANADA.         ", "For EUROPE.                 " };

// ISO9660 stuff:

const char* aCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_!\"%%&'()*+,-./:;<=>?"; //TODO: Define relative to dCharacters?
const char* dCharacters = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789_";

 //TODO: uint8_t and conversion function?
typedef char aCharacter_t;
typedef char dCharacter_t;

typedef struct {
  uint8_t type;
  char magic[5];
  uint8_t version;
  uint8_t data[2041];
} __attribute__((packed)) volumeDescriptor_t;

typedef struct {
  uint8_t length; //Length of Directory Record.
  uint8_t extendedAttributeRecordLength; //	Extended Attribute Record length.
  uint32_t dataLbaLsb; // Location of extent (LBA) in both-endian format.
  uint32_t dataLbaMsb; 
  uint32_t dataLengthLsb; // Data length (size of extent) in both-endian format.
  uint32_t dataLengthMsb;
  uint8_t recordingDateAndTime[7]; // Recording date and time (see format below).

  //uint8_t fileFlags; // File flags (see below).
  union {
    struct {
      uint8_t hidden:1; //  0 	If set, the existence of this file need not be made known to the user (basically a 'hidden' flag.
      uint8_t directory:1; //  1 	If set, this record describes a directory (in other words, it is a subdirectory extent).
      uint8_t associated:1; //  2 	If set, this file is an "Associated File".
      uint8_t formatInExtendedAttributeRecord:1; //  3 	If set, the extended attribute record contains information about the format of this file.
      uint8_t permissionsInExtendedAttributeRecord:1; //  4 	If set, owner and group permissions are set in the extended attribute record.
      uint8_t reserved:2; //  5 & 6 	Reserved
      uint8_t notComplete:1; //If set, this is not the final directory record for this file (for files spanning several extents, for example files over 4GiB long.  
    };
    uint8_t flags;
  };
/*
  26 	1 	File unit size for files recorded in interleaved mode, zero otherwise.
  27 	1 	Interleave gap size for files recorded in interleaved mode, zero otherwise.
  28 	4 	Volume sequence number - the volume that this extent is recorded on, in 16 bit both-endian format.
*/
uint8_t pad[1+1+4];


  uint8_t filenameLength; //  32 	1 	Length of file identifier (file name). This terminates with a ';' character followed by the file ID number in ASCII coded decimal ('1').
  char filename[1]; //  33 	(variable) 	File identifier.
//  (variable) 	1 	Padding field - zero if length of file identifier is odd, otherwise, this field is not present. This means that a directory entry will always start on an even byte number. 
} __attribute__((packed)) directoryEntry_t;

typedef struct {
  uint8_t filenameLength; // Length of Directory Identifier
  uint8_t extendedAttributeRecordLength;
  uint32_t extentLba; // Location of Extent (LBA). This is in a different format depending on whether this is the L-Table or M-Table (see explanation above).
  uint16_t parentDirectoryIndex; //	Directory number of parent directory (an index in to the path table). This is the field that limits the table to 65536 records.
  dCharacter_t filename[1]; // 	(variable) 	Directory Identifier (name) in d-characters.
//  (variable) 	1 	Padding Field - contains a zero if the Length of Directory Identifier field is odd, not present otherwise. This means that each table entry will always start on an even byte number. 
} __attribute__((packed)) pathTableEntry_t;

typedef struct {
  uint8_t zero0; //Always 0x00.
  aCharacter_t systemIdentifier[32]; // The name of the system that can act upon sectors 0x00-0x0F for the volume in a-characters.
  dCharacter_t volumeIdentifier[32]; // Volume Identifier 	Identification of this volume in d-characters.
  uint8_t unused0[8]; // Unused Field 	
  uint32_t volumeSpaceSizeLsb;// Volume Space Size 	Number of Logical Blocks in which the volume is recorded. This is a 32 bit value in both-endian format.
  uint32_t volumeSpaceSizeMsb;
  uint8_t unused1[32]; // Unused Field 	
  uint16_t volumeSetSizeLsb; // Volume Set Size 	The size of the set in this logical volume (number of disks). This is a 16 bit value in both-endian format.
  uint16_t volumeSetSizeMsb;
  uint16_t volumeSequenceNumberLsb; // Volume Sequence Number 	The number of this disk in the Volume Set. This is a 16 bit value in both-endian format.
  uint16_t volumeSequenceNumberMsb;
  uint16_t logicalBlockSizeLsb; // Logical Block Size 	The size in bytes of a logical block in both-endian format. NB: This means that a logical block on a CD could be something other than 2KiB!
  uint16_t logicalBlockSizeMsb;
  uint32_t pathTableSizeLsb; // Path Table Size 	The size in bytes of the path table in 32 bit both-endian format.
  uint32_t pathTableSizeMsb;
  uint32_t lsbPathTableLbaLsb; // Location of Type-L Path Table 	LBA location of the path table, recorded in LSB-first (little endian) format. The path table pointed to also contains LSB-first values.
  uint32_t optionalLsbPathTableLbaLsb; // Location of the Optional Type-L Path Table 	LBA location of the optional path table, recorded in LSB-first (little endian) format. The path table pointed to also contains LSB-first values. Zero means that no optional path table exists.
  uint32_t msbPathTableLbaMsb; // Location of Type-M Path Table 	LBA location of the path table, recorded in MSB-first (big-endian) format. The path table pointed to also contains MSB-first values.
  uint32_t optionalMsbPathTableLbaMsb; // Location of Optional Type-M Path Table 	LBA location of the optional path table, recorded in MSB-first (big-endian) format. The path table pointed to also contains MSB-first values.
  directoryEntry_t rootDirectoryEntry; //Directory entry for the root directory. 	Note that this is not an LBA address, it is the actual Directory Record, which contains a zero-length Directory Identifier, hence the fixed 34 byte size.
/*
  190 	128 	Volume Set Identifier 	Identifier of the volume set of which this volume is a member in d-characters.
  318 	128 	Publisher Identifier 	The volume publisher in a-characters. If unspecified, all bytes should be 0x20. For extended publisher information, the first byte should be 0x5F, followed by an 8.3 format file name. This file must be in the root directory and the filename is made from d-characters.
  446 	128 	Data Preparer Identifier 	The identifier of the person(s) who prepared the data for this volume. Format as per Publisher Identifier.
  574 	128 	Application Identifier 	Identifies how the data are recorded on this volume. Format as per Publisher Identifier.
  702 	38 	Copyright File Identifier 	Identifies a file containing copyright information for this volume set. The file must be contained in the root directory and is in 8.3 format. If no such file is identified, the characters in this field are all set to 0x20.
  740 	36 	Abstract File Identifier 	Identifies a file containing abstract information for this volume set in the same format as the Copyright File Identifier field.
  776 	37 	Bibliographic File Identifier 	Identifies a file containing bibliographic information for this volume set. Format as per the other File Identifier fields.
*/
  uint8_t datesAndTimes[17*4];
  /*
    813 	17 	Volume Creation Date and Time 	Date and Time format as specified below.
    830 	17 	Volume Modification Date and Time 	Date and Time format as specified below.
    847 	17 	Volume Expiration Date and Time 	Date and Time format as specified below. After this date and time, the volume should be considered obsolete. If unspecified, then the information is never considered obsolete.
    864 	17 	Volume Effective Date and Time 	Date and Time format as specified below. Date and time from which the volume should be used. If unspecified, the volume may be used immediately.
  */
  uint8_t one; //File Structure Version 	An 8 bit number specifying the directory records and path table version (always 0x01). //TODO: Rename?
  uint8_t zero1; //Always 0x00.
  uint8_t userContent[512]; // Application Used Contents not defined by ISO 9660.
  uint8_t reserved[653]; // Reserved by ISO. 
} __attribute__((packed)) primaryVolumeDescriptorData_t;

/*
  Volume Descriptor Type Codes:
    0     	Volume descriptor is a Boot Record
    1     	Primary Volume Descriptor
    2 	    Supplementary Volume Descriptor
    3 	    Volume Partition Descriptor
    4-254 	Reserved
    255 	  Volume Descriptor Set Terminator 
*/

typedef struct {
  gdrom_t gdrom;
  volumeDescriptor_t volumeDescriptor;
} chihiroGdrom_t;

/*
The Device Information field begins with a four digit hexadecimal number, which is a CRC on the Product number and Product version fields (16 bytes). Then comes the string " GD-ROM", and finally an indication of how many discs this software uses, and which of these discs that this is. This is indicated by two positive numbers separated with a slash. So if this is the second disc of three, the Device Information string might be "8B40 GD-ROM2/3  ".

The CRC calculation algorithm is as follows (C code):

uint16_t calculateCrc(void* buffer, size_t size) {
  unsigned int i;
  unsigned int c;
  unsigned int n = 0xFFFF;
  uint8_t* data = (uint8_t*)buffer;
  for (i = 0; i < size; i++) {
    n ^= (data[i]<<8);
    for (c = 0; c < 8; c++) {
      if (n & 0x8000) {
      	n = (n << 1) ^ 4129;
      } else {
      	n = (n << 1);
      }
    }
  }
  return n & 0xFFFF;
}
*/

void extractFile(void* base, unsigned int lba, size_t size, char* path) {
  uint8_t* data = (uint8_t*)base;
  printf("Extracting '%s' from lba %i\n",path,lba);
  FILE* file = fopen(path,"wb");
  fwrite(&data[2048 * (lba - 45000)],1,size,file);
  fclose(file);
}

void extractDirectory(void* base, unsigned int lba, size_t size, char* path) {

  // Create the directory  

  mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);

  // Start to fill it

  uint8_t* data = base;
  unsigned int offset = 0;
  printf("Size of directory is %lu\n",size);
  while(offset < size) {
    char newPath[4096]; //TODO: allocate on stack on demand
    directoryEntry_t* rd = (directoryEntry_t*)&data[2048 * (lba - 45000) + offset];
    printf("offset: %i\n", offset);
    offset += rd->length;

    if (rd->filenameLength == 1) {
      if (rd->filename[0] == 0x00) {
        printf("[this directory]\n");
        continue;
      }
      if (rd->filename[0] == 0x01) {
        printf("[parent directory]\n");
        continue;
      }
    }
    if (rd->length == 0) {
      printf("Empty entry!\n");
      break;
    }

    // Cut off the file id if it exists

    char* filenameEnd = &rd->filename[rd->filenameLength];
    char* filenameShortenedEnd = strrchr(rd->filename,';');
    int filenameLength = rd->filenameLength;
    if (filenameShortenedEnd) {
      if (filenameShortenedEnd < filenameEnd) {
        filenameLength = filenameShortenedEnd - rd->filename;
        //TODO: Get fileid
        printf("cut!\n");
      }
    } else {
      printf("[Missing seperator!]");
    }

    // Get filename

    char* filename = alloca(filenameLength + 1);
    strncpy(filename,rd->filename,filenameLength);
    filename[filenameLength] = '\0';

    // Create a path for the entry

    strcpy(newPath,path);
    strcat(newPath,filename);

    // Parse attributes
 
    if (rd->directory) {
      printf("[directory] ");
      strcat(newPath,"/");
      extractDirectory(base, rd->dataLbaLsb,rd->dataLengthLsb,newPath);
    } else {
      if (rd->hidden) {
        printf("[hidden - not extracting] ");
      } else {
        extractFile(base, rd->dataLbaLsb,rd->dataLengthLsb,newPath);
      }
    }
    printf("file '%.*s' (as '%s') 0x%02X, lsb @ 0x%08X, size: %i (0x%08X / 0x%08X)\n",filenameLength,rd->filename,newPath,rd->flags,rd->dataLbaLsb,rd->dataLengthLsb,rd->dataLengthLsb,rd->dataLengthMsb);
  }

}

int main(int argc, char* argv[]) {

  uint8_t* buffer;
  size_t length;
  
  // Parse arguments

  char* source = argv[1];
  char* path = "./gdrom";
  if (argc > 2) {
    path = argv[2];
  }

  // Load the source file

/*
  {
    FILE* bin = fopen(source,"rb");
    fseek(bin,0,SEEK_END);  
    length = ftell(bin);
    fseek(bin,0,SEEK_SET);  
    buffer = malloc(length);
    fread(buffer,length,1,bin);
    fclose(bin);  
    printf("Read %i bytes from file!\n",(int)length);
  }
*/
  int file = open(source, O_RDONLY);
  length = lseek(file, 0, SEEK_END); 
  buffer = mmap(NULL, length, PROT_READ, MAP_PRIVATE, file, 0);
  
  // Show some debug info

  chihiroGdrom_t* gdrom = (chihiroGdrom_t*)buffer;
  printf("Title: '%.128s'\n",gdrom->gdrom.metaInformation.title);
  printf("Product number: '%.10s'\n",gdrom->gdrom.metaInformation.productNumber);
  printf("Product version: '%.6s'\n",gdrom->gdrom.metaInformation.productVersion);
  printf("Release date: '%.16s'\n",gdrom->gdrom.metaInformation.releaseDate);

  // Get descriptor

  //TODO: Load these in a loop until a terminator is found (type = 0xFF)
  if (gdrom->volumeDescriptor.type != 0x01) {
    printf("Expected primary volume descriptor in sector 16! (offset: 0x%lX)\n",(uintptr_t)&gdrom->volumeDescriptor - (uintptr_t)gdrom);
    return 1;
  }
  primaryVolumeDescriptorData_t* primaryVolumeDescriptorData = (primaryVolumeDescriptorData_t*)&gdrom->volumeDescriptor.data;
  printf("System identifier: '%.32s'\n",primaryVolumeDescriptorData->systemIdentifier);
  printf("Volume identifier: '%.32s'\n",primaryVolumeDescriptorData->volumeIdentifier);
  printf("Set: %u / %u\n",primaryVolumeDescriptorData->volumeSequenceNumberLsb,primaryVolumeDescriptorData->volumeSetSizeLsb);
  printf("LB-Size: %u (0x%08X)\n",primaryVolumeDescriptorData->logicalBlockSizeLsb,primaryVolumeDescriptorData->logicalBlockSizeMsb);
  printf("LB-Size offset: %lu\n",(uintptr_t)&primaryVolumeDescriptorData->logicalBlockSizeLsb - (uintptr_t)&gdrom->volumeDescriptor);
  printf("LSB Path Table Size: %u\n",primaryVolumeDescriptorData->pathTableSizeLsb);
  printf("LSB Path Table LBA: %u\n",primaryVolumeDescriptorData->lsbPathTableLbaLsb);
  pathTableEntry_t* pathTable = (pathTableEntry_t*)&buffer[2048*(45018-45000)]; //TODO: Use variables instead, blocksize and pathtable lba
//  printf("Got file: '%s'\n",pathTable->filename);

  char newPath[4096];
  strcpy(newPath,path);
  if (newPath[strlen(newPath) - 1] != '/') {
    strcat(newPath,"/");
  }
  extractDirectory(buffer, primaryVolumeDescriptorData->rootDirectoryEntry.dataLbaLsb,primaryVolumeDescriptorData->rootDirectoryEntry.dataLengthLsb,newPath);


  printf("Root has '%s' (LBA is %i)\n",primaryVolumeDescriptorData->rootDirectoryEntry.filename,primaryVolumeDescriptorData->rootDirectoryEntry.extendedAttributeRecordLength);
  
}
