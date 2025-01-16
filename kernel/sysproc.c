#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  argint(0, &n);
  exit(n);
  return 0; // not reached
}

uint64
sys_getpid(void)
{
  return myproc()->pid;
}

uint64
sys_fork(void)
{
  return fork();
}

uint64
sys_wait(void)
{
  uint64 p;
  argaddr(0, &p);
  return wait(p);
}

uint64
sys_sbrk(void)
{
  uint64 addr;
  int n;

  argint(0, &n);
  addr = myproc()->sz;
  if (growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;

  argint(0, &n);
  if (n < 0)
    n = 0;
  acquire(&tickslock);
  ticks0 = ticks;
  while (ticks - ticks0 < n)
  {
    if (killed(myproc()))
    {
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}

uint64
sys_kill(void)
{
  int pid;

  argint(0, &pid);
  return kill(pid);
}

// return how many clock tick interrupts have occurred
// since start.
uint64
sys_uptime(void)
{
  uint xticks;

  acquire(&tickslock);
  xticks = ticks;
  release(&tickslock);
  return xticks;
}

// uint64
// sys_calculate(void)
// {
//   int x = 0, y = 0;
//   char op = 0;
//   int *result = 0;
//   int calc_result = 0;

//   // if (argint(0, &x) < 0 ||
//   //     argint(1, &y) < 0 ||
//   //     argaddr(3, (uint64 *)&result) < 0)
//   // {
//   //   return -1;
//   // }

//   // char *op_addr;
//   // if (argaddr(2, (uint64 *)&op_addr) < 0 ||
//   //     copyin(myproc()->pagetable, &op, op_addr, sizeof(char)) < 0)
//   // {
//   //   return -1;
//   // }

//   switch (op)
//   {
//   case '+':
//     calc_result = x + y;
//     break;
//   case '-':
//     calc_result = x - y;
//     break;
//   case '*':
//     calc_result = x * y;
//     break;
//   case '/':
//     if (y == 0)
//       return -1;
//     calc_result = x / y;
//     break;
//   default:
//     return -1;
//   }

//   if (copyout(myproc()->pagetable, (uint64)result, (char *)&calc_result, sizeof(int)) < 0)
//   {
//     return -1;
//   }

//   return 0;
// }
