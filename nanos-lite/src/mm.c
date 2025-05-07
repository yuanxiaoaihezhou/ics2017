#include "proc.h"
#include "memory.h"

static void *pf = NULL;

void* new_page(void) {
  assert(pf < (void *)_heap.end);
  void *p = pf;
  pf += PGSIZE;
  return p;
}

void free_page(void *p) {
  panic("not implement yet");
}

/* The brk() system call handler. */
int mm_brk(uint32_t new_brk) {
  if (current->cur_brk == 0) {
    current->cur_brk = current->max_brk = new_brk;
  } else {
    if (new_brk > current->max_brk) {
      // 计算当前最大brk的4K对齐地址
      uint32_t aligned_brk = (current->max_brk + 0xfff) & ~0xfff;
      // 逐页映射虚拟地址空间
      for (uint32_t brk = aligned_brk; brk < new_brk; brk += PGSIZE) {
        void* pg = new_page();  // 获取新的物理页
        _map(&current->as, (void*)brk, pg);  // 建立映射
      }
      current->max_brk = new_brk;  // 更新历史最大brk
    }
  current->cur_brk = new_brk;  // 更新当前brk
  }
  return 0;
}

void init_mm() {
  pf = (void *)PGROUNDUP((uintptr_t)_heap.start);
  Log("free physical pages starting from %p", pf);

  _pte_init(new_page, free_page);
}
