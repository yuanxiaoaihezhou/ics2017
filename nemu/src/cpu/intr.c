#include "cpu/exec.h"
#include "memory/mmu.h"

void raise_intr(uint8_t NO, vaddr_t ret_addr)
{
  /* TODO: Trigger an interrupt/exception with ``NO''.
   * That is, use ``NO'' to index the IDT.
   */

  vaddr_t gate_addr = cpu.idtr.base + NO * 8;

  if (cpu.idtr.limit < 0)
  {
    assert(0);
  }

  uint8_t attr = vaddr_read(gate_addr + 5, 1); 
  if (!(attr & 0x80))
  {            
    assert(0); 
  }

  uint16_t offset_low = vaddr_read(gate_addr + 0, 2);  
  uint16_t offset_high = vaddr_read(gate_addr + 6, 2); 
  uint32_t entry_point = (offset_high << 16) | offset_low;

  rtl_push(&cpu.eflags); 
  rtl_push(&cpu.cs);     
  rtl_push(&ret_addr);                

  decoding.jmp_eip = entry_point;
  decoding.is_jmp = true;
}

void dev_raise_intr()
{
}
