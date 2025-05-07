#include "nemu.h"
#include "device/mmio.h"
#include "memory/mmu.h"

#define PMEM_SIZE (128 * 1024 * 1024)

#define pmem_rw(addr, type) *(type *)({                                       \
  Assert(addr < PMEM_SIZE, "physical address(0x%08x) is out of bound", addr); \
  guest_to_host(addr);                                                        \
})

uint8_t pmem[PMEM_SIZE];

/* Memory accessing interfaces */

uint32_t paddr_read(paddr_t addr, int len)
{
  int map_NO = is_mmio(addr);
  return (map_NO == -1) ? (pmem_rw(addr, uint32_t) & (~0u >> ((4 - len) << 3))) : mmio_read(addr, len, map_NO);
}

void paddr_write(paddr_t addr, int len, uint32_t data)
{
  int map_NO = is_mmio(addr);
  if (map_NO != -1)
  {
    mmio_write(addr, len, data, map_NO);
  }
  else
  {
    memcpy(guest_to_host(addr), &data, len);
  }
}

uint32_t vaddr_read(vaddr_t addr, int len) {
  if ((((addr) + (len) - 1) & ~PAGE_MASK) != ((addr) & ~PAGE_MASK)) {
    uint32_t data = 0;
    for (int i = 0; i < len; i++) {
      paddr_t paddr = page_translate(addr + i, 0);
      data += paddr_read(paddr, 1) << (i * 8);
    }
    return data;
  } else {
    paddr_t paddr = page_translate(addr, 0);
    return paddr_read(paddr, len);
  }
}

void vaddr_write(vaddr_t addr, int len, uint32_t data) {
  if ((((addr) + (len) - 1) & ~PAGE_MASK) != ((addr) & ~PAGE_MASK)) {
    for (int i = 0; i < len; i++) {
      paddr_t paddr = page_translate(addr + i, 1);
      paddr_write(paddr, 1, data >> (i * 8));
    }
  } else {
    paddr_t paddr = page_translate(addr, 1);
    paddr_write(paddr, len, data);
  }
}

paddr_t page_translate(vaddr_t addr, int rw) {
    if (!cpu.cr0.protect_enable || !cpu.cr0.paging)
        return addr;

    const uint32_t offset = addr & 0xFFF;  
    PDE *const pd = (PDE *)(uintptr_t)(cpu.cr3.val & 0xFFFFF000);  // 获取页目录基址
    const int pd_index = (addr >> 22) & 0x3FF; 

    // 处理页目录项
    PDE pde;
    pde.val = paddr_read((paddr_t)&pd[pd_index], 4);
    if (!pde.present)
        Assert(0, "pde present bit is 0!");

    PTE *const pt = (PTE *)(uintptr_t)(pde.val & ~0xFFF);  // 页表基址
    const uint32_t old_pde_val = pde.val;
    pde.accessed = 1;
    if (pde.val != old_pde_val)  
        paddr_write((paddr_t)&pd[pd_index], 4, pde.val);

    // 处理页表项
    const int pt_index = (addr >> 12) & 0x3FF;  // 页表索引
    PTE pte;
    pte.val = paddr_read((paddr_t)&pt[pt_index], 4);
    if (!pte.present)
        Assert(0, "pte present bit is 0!");

    const uint32_t old_pte_val = pte.val;
    pte.accessed = 1;
    if (rw) pte.dirty = 1;       
    if (pte.val != old_pte_val)  
        paddr_write((paddr_t)&pt[pt_index], 4, pte.val);

    return (pte.val & ~0xFFF) | offset;  
}