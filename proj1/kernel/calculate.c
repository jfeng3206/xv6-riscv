#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "kernel/syscall.h"
#include "kernel/defs.h"

uint64 sys_calculate(void)
{
  int x, y;
  uint64 op_addr;
  uint64 result;
  int calc_result;
  char op;

  argint(0, &x);
  argint(1, &y);
  argaddr(2, &op_addr);
  argaddr(3, &result);

  copyin(myproc()->pagetable, &op, op_addr, 1);

  switch (op)
  {
  case '+':
    calc_result = x + y;
    break;
  case '-':
    calc_result = x - y;
    break;
  case '*':
    calc_result = x * y;
    break;
  case '/':
    if (y == 0)
      return -1;
    calc_result = x / y;
    break;
  default:
    return -1;
  }
  if (copyout(myproc()->pagetable, (uint64)result, (char *)&calc_result, sizeof(int)) < 0)
  {
    return -1;
  }
  return calc_result;
}
