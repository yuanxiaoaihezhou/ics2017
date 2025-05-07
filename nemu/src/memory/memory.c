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

uint32_t vaddr_read(vaddr_t addr, int len)
{
  return paddr_read(addr, len);
}

void vaddr_write(vaddr_t addr, int len, uint32_t data)
{
  paddr_write(addr, len, data);
}

paddr_t page_translate(vaddr_t addr, int rw) {
    if (!cpu.cr0.protect_enable || !cpu.cr0.paging)
        return addr;

    const uint32_t PDE_PRESENT = 0x1;
    const uint32_t PDE_ACCESSED = 0x20;
    const uint32_t PTE_PRESENT = 0x1;
    const uint32_t PTE_ACCESSED = 0x20;
    const uint32_t PTE_DIRTY = 0x40;

    paddr_t pd_base = cpu.cr3.val & 0xfffff000;
    int pd_index = (addr >> 22) & 0x3ff;
    paddr_t pde_addr = pd_base + (pd_index << 2); // pd_index * 4

    uint32_t pde_val = paddr_read(pde_addr, 4);
    if (!(pde_val & PDE_PRESENT))
        Assert(0, "PDE present bit is 0!");

    if (!(pde_val & PDE_ACCESSED)) {
        pde_val |= PDE_ACCESSED;
        paddr_write(pde_addr, 4, pde_val);
    }

    paddr_t pt_base = pde_val & 0xfffff000;
    int pt_index = (addr >> 12) & 0x3ff;
    paddr_t pte_addr = pt_base + (pt_index << 2); // pt_index * 4

    uint32_t pte_val = paddr_read(pte_addr, 4);
    if (!(pte_val & PTE_PRESENT))
        Assert(0, "PTE present bit is 0!");

    paddr_t phys_addr = (pte_val & 0xfffff000) | (addr & 0xfff);

    uint32_t new_pte_val = pte_val;
    bool need_write = false;
    
    if (!(pte_val & PTE_ACCESSED)) {
        new_pte_val |= PTE_ACCESSED;
        need_write = true;
    }
    if (rw && !(pte_val & PTE_DIRTY)) {
        new_pte_val |= PTE_DIRTY;
        need_write = true;
    }
    
    if (need_write) {
        paddr_write(pte_addr, 4, new_pte_val);
    }

    return phys_addr;
}