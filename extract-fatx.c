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

const unsigned int bytesPerSector = 512;
const unsigned int rootDirectoryEntryCount = 256;

typedef struct {
  int8_t magic[4];
  uint32_t volumeId;
  uint32_t sectorsPerCluster;
  uint32_t firstRootCluster;
  uint8_t unused[4080];
} __attribute__((packed)) superblock_t;

typedef void* fatEntry_t; // Dummy
typedef uint16_t fat16Entry_t;
typedef uint32_t fat32Entry_t;

typedef struct {
  superblock_t superblock;
  union {
    fatEntry_t fatEntries[1];
    fat16Entry_t fat16Entries[1];
    fat32Entry_t fat32Entries[1];
  };
} __attribute__((packed)) fatx_t;

typedef struct {
  uint8_t filenameSize;
  uint8_t attributes;
  char filename[42]; //TODO: Meh char seems sort of unportable - should be changed
  uint32_t firstCluster;
  uint32_t fileSize;
  /*52 	2 	Modification time
  54 	2 	Modification date
  56 	2 	Creation time
  58 	2 	Creation date
  60 	2 	Last access time
  62 	2 	Last access date 
  */
  uint16_t timesAndDates[6];
} __attribute__((packed)) directoryEntry_t;

/*
  0x0000          0x00000000 	            Free Cluster — also used as parent directory starting cluster in ".." entries for subdirectories of the root directory (FAT12/16)
  0x0001 	        0x00000001 	            Reserved, do not use
  0x0002‑0xFFEF 	0x00000002‑0x0FFFFFEF 	Used cluster; value points to next cluster
  0xFFF0‑0xFFF5 	0x0FFFFFF0‑0x0FFFFFF5 	Reserved in some contexts[11] or also used[3][9][10]
  0xFFF6        	0x0FFFFFF6            	Reserved; do not use[3][9][10][29]
  0xFFF7 	        0x0FFFFFF7             	Bad sector in cluster or reserved cluster
  0xFFF8‑0xFFFF 	0x0FFFFFF8‑0x0FFFFFFF 	Last cluster in file (EOC)
*/

void extractFile(bool fat16, fatEntry_t* fatEntries, void* clusterBase, size_t clusterSize, unsigned int cluster, char* path, unsigned int fileSize) {
  uint8_t* clusterData = (uint8_t*)clusterBase;
  printf("Extracting '%s' from cluster %i\n",path,cluster);
  FILE* file = fopen(path,"wb");
  if (file == NULL) {
    printf("File could not be opened for writing! This should not happen..\n");
    return;
  }
  while((cluster < fat16?0xFFF0:0x0FFFFFF0) && fileSize) {
    void* clusterContent = &clusterData[clusterSize * cluster];
    size_t chunkSize;
    if (fileSize < clusterSize) {
      chunkSize = fileSize;
    } else {
      chunkSize = clusterSize;
      if (fat16) {
        fat16Entry_t* fat16Entries = (fat16Entry_t*)fatEntries;
        cluster = fat16Entries[cluster];
      } else {
        fat32Entry_t* fat32Entries = (fat32Entry_t*)fatEntries;
        cluster = fat32Entries[cluster];
      }
    }
    fwrite(clusterContent,1,chunkSize,file);
    fileSize -= chunkSize;
//    printf("Wrote one chunk (0x%X bytes), now going to cluster %i, %i bytes left\n",(unsigned int)chunkSize,cluster,fileSize);
  }
  fclose(file);
}

void extractDirectory(bool fat16, fatEntry_t* fatEntries, void* clusterBase, size_t clusterSize, unsigned int cluster, char* path, unsigned int entryCount) {

  // Create the directory  

  mkdir(path, S_IRWXU | S_IRWXG | S_IRWXO);

  // Start to fill it

  uint8_t* clusterData = (uint8_t*)clusterBase;
  directoryEntry_t* entries = (directoryEntry_t*)&clusterData[cluster * clusterSize];
  unsigned int i;
  for(i = 0; i < clusterSize / sizeof(directoryEntry_t); i++) {

    if (i >= entryCount) {
      printf("Too many files in directory?!\n");
      usleep(5000*1000);
      if (0) {
        printf("Directory stopped!\n");
        break;
      } else {
        printf("Ignoring limit!\n");
      }
    }

    char newPath[4096]; //TODO: allocate on stack on demand
    directoryEntry_t* entry = &entries[i];

    if ((entry->filenameSize == 0x00) || (entry->filenameSize == 0xFF)) {
      printf("[end of directory]\n");
      break;
    }

    if (entry->filenameSize == 0xE5) {
      printf("[deleted file]\n");
    } else {

      // Get filename

      char filename[42+1];
      strncpy(filename,entry->filename,entry->filenameSize);
      filename[entry->filenameSize] = '\0';

      // Create a path for the entry

      strcpy(newPath,path);
      strcat(newPath,filename);

      // Debug message

      printf("Entry %i (Cluster %i) is called 0x%X '%.*s' (extracting to '%s'), firstcluster is %i (Filesize: %u bytes)\n",i,cluster,entry->filenameSize,entry->filenameSize,entry->filename,newPath,entry->firstCluster,entry->fileSize);
      if (fat16) {
        fat16Entry_t* fat16Entries = (fat16Entry_t*)fatEntries;
        printf("Cluster type: 0x%04X\n",fat16Entries[cluster]);
      } else {
        fat32Entry_t* fat32Entries = (fat32Entry_t*)fatEntries;
        printf("Cluster type: 0x%08X\n",fat32Entries[cluster]);
      }

      if (entry->firstCluster < 2) {
        printf("Entry in free cluster detected! Attributes: 0x%02X\n",entry->attributes);
        continue;
      }

      // Check if new attributes are used

      if (entry->attributes & (~0xBF)) { 
        printf("Unknown attributes included: 0x%02X!\n",entry->attributes);
        while(1);
      }

      // Print all attributes
      if (entry->attributes & 0x01) { 
        printf("Read-only file?\n");
      }
      if (entry->attributes & 0x02) { 
        printf("Hidden file?\n");
				usleep(1000*1000);
      }
      if (entry->attributes & 0x04) { 
        printf("System file?\n");
				usleep(1000*1000);
      }
      if (entry->attributes & 0x20) { 
        printf("Archive?\n");
      }
      if (entry->attributes & 0x80) { 
        printf("Normal file?\n");
      }
    
      // Extract file or directory

      if (entry->attributes & 0x08) { 
		    if (entry->attributes & 0x10) {
					printf("This is a long filename component which does not exist on xbox? Stopping.\n");
					while(1);
				}
        printf("Volume label\n");
      } else {
		    if (entry->attributes & 0x10) {
		      printf("Trying to open dir\n");
		      strcat(newPath,"/");
		      extractDirectory(fat16, fatEntries, clusterBase, clusterSize, entry->firstCluster, newPath, 4096);          
		      //TODO: what if this cluster has a following one?
		    } else {
					if (entry->fileSize > 0) {
			      extractFile(fat16, fatEntries, clusterBase, clusterSize, entry->firstCluster, newPath, entry->fileSize);
					}
		    }
			}

    }
  }

  entryCount -= i;
  
  printf("Leaving directory\n");
  if (fat16) {
    fat16Entry_t* fat16Entries = (fat16Entry_t*)fatEntries;
    printf("Cluster type: 0x%04X\n",fat16Entries[cluster]);
    if (fat16Entries[cluster] != 0xFFFF) {
      extractDirectory(fat16, fatEntries, clusterBase, clusterSize, fat16Entries[cluster], path, entryCount);
      /*printf("Directory not entirely read! Stopping\n");
      while(1);*/
    }
  } else {
    fat32Entry_t* fat32Entries = (fat32Entry_t*)fatEntries;
    printf("Cluster type: 0x%08X\n",fat32Entries[cluster]);
    if (fat32Entries[cluster] != 0xFFFFFFFF) {
      extractDirectory(fat16, fatEntries, clusterBase, clusterSize, fat32Entries[cluster], path, entryCount);
      /*printf("Directory not entirely read! Stopping\n");
      while(1);*/
    }
  }

  return;

}

int main(int argc, char* argv[]) {

  uint8_t* buffer;
  size_t length;
  
  // Parse arguments
  
  if (argc < 3) {
    printf("%s <source> <destination> [<partition size in bytes>]\n",argv[0]);
    return 1;
  }

  char* source = argv[1];
  char* path = argv[2];
  char* size = "";
  if (argc > 3) {
    size = argv[3];
    printf("Attempting to use explicit partition size!\n");
  }

  // Load the source file

/*
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
*/
  int file = open(source, O_RDONLY);
  length = lseek(file, 0, SEEK_END); 
  buffer = mmap(NULL, length, PROT_READ, MAP_PRIVATE, file, 0);

  fatx_t* fatx = (fatx_t*)buffer;  

  unsigned int partitionSize = length;

//	printf("Guessed partition size: %u bytes\n",(fatx->superblock.sectorsPerCluster==32)?1024000000:512000000);

  if (sscanf(size, "0x%X", &partitionSize) != 1) {
		sscanf(size, "%u", &partitionSize);
  }


  size_t fatEntrySize;
  size_t clusterSize = fatx->superblock.sectorsPerCluster * bytesPerSector;
  unsigned int clusterCount = partitionSize / clusterSize + 1;
  bool fat16 = clusterCount < 0xFFF0;
  if (fat16) {
    fatEntrySize = sizeof(fat16Entry_t);
  } else {
    fatEntrySize = sizeof(fat32Entry_t);
  }

  size_t fatSize = clusterCount * fatEntrySize;
  printf("FAT size is %lu / 0x%lX\n",fatSize,fatSize); 

//crtaxihr: 50880 fatsize measured
//wangmid2: 495152 fatsize measured

/*

	size_t fatxSize = fatSize;
	  fatxSize += (0x1000-(fatxSize % 0x1000)) % 0x1000; // Xbox-Linux describes this with 0x1000 alignment, I chose a cluster.. only fat16 sticks to 0x1000?


//	  fatxSize += (0x10000-(fatxSize % 0x10000)) % 0x10000; // Xbox-Linux describes this with 0x1000 alignment, I chose a cluster.. only fat16 sticks to 0x1000?

//wangmid2:
//		fatxSize += ((clusterSize*4)-(fatxSize % (clusterSize*4))) % (clusterSize*4); // Xbox-Linux describes this with 0x1000 alignment, I chose a cluster..

*/

//   size_t fatxSize = (fatSize + (0x1000 - 1)) & ~(0x1000 - 1);
  

    

  printf("Partition-Size: %u bytes\n",partitionSize);
  printf("Cluster sector count is %u\n",fatx->superblock.sectorsPerCluster);
  printf("First root cluster: %u\n",fatx->superblock.firstRootCluster);
  printf("Guessed clusters count: %u / 0x%X%s\n",clusterCount,clusterCount,fat16?" (FAT16)":"");
//((69632-4096)/2)*16384 = ps
	printf("Superblock is %lu / 0x%lX bytes\n",sizeof(superblock_t),sizeof(superblock_t));
//  printf("Aligned FAT size is %lu / 0x%lX bytes\n",fatxSize,fatxSize);
  printf("Cluster size is %lu / 0x%lX bytes\n",clusterSize,clusterSize);

  printf("Searching file area...\n");

  uint8_t* clusterBase = (uint8_t*)(((uintptr_t)fatx)  + sizeof(superblock_t) + fatSize); // All clusters are starting at 1, we fix this by moving our array one index down
  while(*clusterBase == 0x00) {
    clusterBase++;
  }
  clusterBase = (uint8_t*)((uintptr_t)clusterBase - clusterSize);

  printf("Cluster base is %lu / 0x%lX bytes\n",(uintptr_t)clusterBase - (uintptr_t)fatx,(uintptr_t)clusterBase - (uintptr_t)fatx);

  char newPath[4096];
  strcpy(newPath,path);
  if (newPath[strlen(newPath) - 1] != '/') {
    strcat(newPath,"/");
  }
  extractDirectory(fat16, fatx->fatEntries, clusterBase, clusterSize, fatx->superblock.firstRootCluster, newPath, clusterSize / sizeof(directoryEntry_t)); //TODO: Why do I have to advance one cluster? Why isn't this in the first one (which would be 0)?

  printf("Done!\n");

  return 0;
}
