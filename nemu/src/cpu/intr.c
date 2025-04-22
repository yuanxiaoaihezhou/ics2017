#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr)
{
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  vaddr_t gate_addr = cpu.idtr.base + 8 * NO;

  if (cpu.idtr.limit < 0)
  {
    assert(0);
  }

  uint16_t offset_low = vaddr_read(gate_addr + 0, 2); 
  uint16_t offset_high = vaddr_read(gate_addr + 6, 2); 
  uint32_t entry_point = (offset_high << 16) | offset_low;

  rtl_push((uint32_t[]){cpu.eflags}); // EFLAGS
  rtl_push((uint32_t[]){cpu.cs});     // CS（16位扩展为32位）
  rtl_push(&ret_addr);                // 返回地址

  decoding.jmp_eip = entry_point;
  decoding.is_jmp = true;
}

void dev_raise_intr()
{
}
