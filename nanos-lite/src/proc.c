#include "proc.h"

#define MAX_NR_PROC 4

static PCB pcb[MAX_NR_PROC];
static int nr_proc = 0;
PCB *current = NULL;

uintptr_t loader(_Protect *as, const char *filename);

void load_prog(const char *filename) {
  int i = nr_proc ++;
  _protect(&pcb[i].as);

  uintptr_t entry = loader(&pcb[i].as, filename);

  // TODO: remove the following three lines after you have implemented _umake()
  // _switch(&pcb[i].as);
  // current = &pcb[i];
  // ((void (*)(void))entry)();

  _Area stack;
  stack.start = pcb[i].stack;
  stack.end = stack.start + sizeof(pcb[i].stack);

  pcb[i].tf = _umake(&pcb[i].as, stack, stack, (void *)entry, NULL, NULL);
}

extern bool current_game;
_RegSet* schedule(_RegSet *prev) {
  if(current != NULL){
    current->tf = prev;
  }
  else{
    current = &pcb[0];
  }
  static int num = 0;
  if(num == 100){
    current = &pcb[1];
    num = 0;
  }
  else{
    if(current_game)
    {
      current = &pcb[0];
      num++;
    }
    else
    {
      current = &pcb[2];
      num++;
    }
  }
  _switch(&current->as);
  return current->tf;
}
