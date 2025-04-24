#include "cpu/rtl.h"

/* Condition Code */

void rtl_setcc(rtlreg_t *dest, uint8_t subcode)
{
  bool invert = subcode & 0x1;
  enum
  {
    CC_O,
    CC_NO,
    CC_B,
    CC_NB,
    CC_E,
    CC_NE,
    CC_BE,
    CC_NBE,
    CC_S,
    CC_NS,
    CC_P,
    CC_NP,
    CC_L,
    CC_NL,
    CC_LE,
    CC_NLE
  };
  switch (subcode & 0xe)
  {
  case CC_O: //0
    *dest = cpu.OF;
    break;
  case CC_B: //2
    *dest = cpu.CF;
    break;
  case CC_E: //4
    *dest = cpu.ZF;
    break;
  case CC_BE: //6
    *dest = ((cpu.CF) || (cpu.ZF));
    break;
  case CC_S: //8
    *dest = cpu.SF;
    break;
  case CC_L: //12 c
    *dest = (cpu.SF != cpu.OF);
    break;
  case CC_LE: //14 e
    *dest = ((cpu.ZF) || (cpu.SF != cpu.OF));
    break;
  default:
    panic("should not reach here");
  case CC_P:
    panic("n86 does not have PF");
  }

  if (invert)
  {
    rtl_xori(dest, dest, 0x1);
  }
}