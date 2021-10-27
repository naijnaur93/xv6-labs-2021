#include "types.h"
#include "riscv.h"
#include "param.h"
#include "defs.h"
#include "date.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  exit(n);
  return 0;  // not reached
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
  if(argaddr(0, &p) < 0)
    return -1;
  return wait(p);
}

uint64
sys_sbrk(void)
{
  int addr;
  int n;

  if(argint(0, &n) < 0)
    return -1;
  
  addr = myproc()->sz;
  if(growproc(n) < 0)
    return -1;
  return addr;
}

uint64
sys_sleep(void)
{
  int n;
  uint ticks0;


  if(argint(0, &n) < 0)
    return -1;
  acquire(&tickslock);
  ticks0 = ticks;
  while(ticks - ticks0 < n){
    if(myproc()->killed){
      release(&tickslock);
      return -1;
    }
    sleep(&ticks, &tickslock);
  }
  release(&tickslock);
  return 0;
}


//#ifdef LAB_PGTBL
int
sys_pgaccess(void)
{
  // lab pgtbl: your code here.
  uint64 va;   // the starting virtual address
  if (argaddr(0, &va) < 0) {
    panic("sys_pgaccess: failure to get virtual addr");
  }
  int pgnum;   // the number of pages to check
  if (argint(1, &pgnum) < 0) {
    panic("sys_pgaccess: failure to get pg num");
  }
  if (pgnum > 64) {
    panic("too much pages to check!");
  }
  uint64 ubuf;  // the address of user buffer
  if (argaddr(2, &ubuf) < 0) {
    panic("sys_pgaccess: failure to get addr of user buffer");
  }
  uint64 buf = 0; // the output bit mask
  for (int i = 0; i < pgnum; i++)
  {
    pte_t *pte_ptr = walk(myproc()->pagetable, va + i*PGSIZE, 0);
    if (*pte_ptr & PTE_A) {  // the page which the PTE points to is accessed
      buf |= (1L << i);      // set the output bit mask
      *pte_ptr &= ~PTE_A;    // clear the access bit(operator~ means flop every bit)
    }
  }
  // copy the buf to ubuf
  if (copyout(myproc()->pagetable, ubuf, (char *)&buf, 8) < 0)
  {
    panic("error: copyout() in sys_pgaccess()");
  }

  return 0;
}
//#endif

uint64
sys_kill(void)
{
  int pid;

  if(argint(0, &pid) < 0)
    return -1;
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
