#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({\
    Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
    guest_to_host(addr); \
    })

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len) {
  int map_NO = is_mmio(addr);
  return (map_NO == -1) ? (pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3))) : mmio_read(addr, len, map_NO);
}

void paddr_write(paddr_t addr, int len, uint32_t data) {
  int map_NO = is_mmio(addr);
  if (map_NO == -1) {
    uint8_t *p = (uint8_t *)guest_to_host(addr);
    for (int i = 0; i < len; ++i) {
      p[i] = (data >> (i * 8)) & 0xff;
    }
  } else {
    mmio_write(addr, len, data, map_NO);
  }
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  paddr_write(addr, len, data);
}
