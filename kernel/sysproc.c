#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "date.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "proc.h"
#include "fcntl.h"

struct file {
  enum { FD_NONE, FD_PIPE, FD_INODE, FD_DEVICE } type;
  int ref;  // reference count
  char readable;
  char writable;
  struct pipe *pipe;  // FD_PIPE
  struct inode *ip;   // FD_INODE and FD_DEVICE
  uint off;           // FD_INODE
  short major;        // FD_DEVICE
};

uint64 munmap(uint64 va, uint64 length);

uint64
sys_exit(void)
{
  int n;
  if(argint(0, &n) < 0)
    return -1;
  int i;
  struct proc *p = myproc();
  // unmap all vmas
  for (i = 0; i < MAXVMA; i++) {
    if (p->vmas[i].f) {
      munmap(p->vmas[i].addr, p->vmas[i].length);
    }
  }
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

uint64 sys_mmap(void) {
  uint64 addr, length, offset;
  int flags, prot, fd;
  int i;
  struct proc *p = myproc();

  if (argaddr(0, &addr) < 0 || argaddr(1, &length) < 0 || argint(2, &prot) < 0 ||
      argint(3, &flags) < 0 || argint(4, &fd) < 0 || argaddr(5, &offset) < 0) {
    panic("mmap(), fail to retrieve some arguments!");
  }
  
  if (flags & MAP_SHARED) {
    // check if there's conflicting PROT and file permission
    if ((prot & PROT_READ) && !p->ofile[fd]->readable) {
      printf("mmap(), mapped as PROT_READ, but file not readable!\n");
      return -1;
    }
    if ((prot & PROT_WRITE) && !p->ofile[fd]->writable) {
      printf("mmap(), mapped as PROT_WRITE, but file not writeable!\n");
      return -1;
    }
  }

  // allocate a vma for the mmap
  for (i = 0; i < MAXVMA; i++) {
    if (p->vmas[i].f == 0) {
      // the vma is available
      break;
    }
  }
  if (i == MAXVMA) return 0xffffffffffffffff;  // no free vma
  if (addr == 0) {
    // find an unused memory range to map the file
    addr = PGROUNDUP(p->sz);
  }
  p->sz = addr + PGROUNDUP(length);
  // fill out the vma info
  p->vmas[i].addr = addr;
  p->vmas[i].flags = flags;
  p->vmas[i].length = length;
  p->vmas[i].offset = offset;
  p->vmas[i].prot = prot;
  p->vmas[i].f = p->ofile[fd];
  // printf("mmap: vma start addr: %p, length: %d, p->sz = %d\n", p->vmas[i].addr, p->vmas[i].length, p->sz);

  // map these vma pages. Since we apply lazy alloc, now pa is not available
  if (mappages(p->pagetable, addr, length, 0, PTE_M) < 0)
    return 0xffffffffffffffff;
  // increase the file's ref count
  filedup(p->vmas[i].f);
  return addr;
}

void vmaunmap(pagetable_t pagetable, uint64 va, uint64 npages, struct Vma *vma) {
  uint64 a;
  pte_t *pte;
  if ((va % PGSIZE) != 0) panic("vmaunmap: not aligned");
  for (a = va; a < va + npages * PGSIZE; a += PGSIZE) {
    if ((pte = walk(pagetable, a, 0)) == 0) panic("vmaunmap: walk");
    if ((*pte & PTE_V) == 0) panic("vmaunmap: not mapped");
    if (PTE_FLAGS(*pte) == PTE_V) panic("vmaunmap: not a leaf");
    // flush and free phisical memory only if the vma is allocated
    // PTE_U bit in the VMA means it has been allocated physical memory
    if (*pte & PTE_U) {
      if ((vma->flags & MAP_SHARED) && (*pte & PTE_D)) {
        // flush the page if it's dirty
        begin_op();
        ilock(vma->f->ip);
        writei(vma->f->ip, 1, a, vma->offset + a - vma->addr, PGSIZE);
        iunlock(vma->f->ip);
        end_op();
      }
      // free the memory
      kfree((void *)PTE2PA(*pte));
    }
    *pte = 0;
  }
}

uint64 munmap(uint64 va, uint64 length) {
  struct proc *p = myproc();
  int i = 0;

  // find the vma for the address range
  va = PGROUNDDOWN(va);
  for (; i < MAXVMA; i++) {
    if (p->vmas[i].addr + p->vmas[i].length > va) break;
  }
  if (i == MAXVMA || p->vmas[i].addr > va) {
    printf("munmap(): No matched vma for va: %p\n", va);
    return -1;
  }

  length = PGROUNDUP(length);
  struct Vma *vma = &p->vmas[i];
  // Do not accept holes
  // Either `va+length == the end of vma`, or `va == the begin of vma`
  if ((va + length == vma->addr + vma->length) || (va == vma->addr)) {
    // unmap the pages
    vmaunmap(p->pagetable, va, length / PGSIZE, vma);
    if (length == vma->length) {
      // munmap removes all pages, decrease file's ref count
      fileclose(vma->f);
      // free this vma
      vma->f = 0;
    }
    // update vma start addr and length
    if (va == vma->addr) vma->addr += length;
    vma->length -= length;
    // printf("munmap(): vma start addr: %p, length: %d\n", va, vma->length);

    return 0;
  }
  printf("There's a hole in munmap()\n");
  return -1;
}

uint64 sys_munmap(void) {
  uint64 va, length;

  if (argaddr(0, &va) < 0 || argaddr(1, &length) < 0) {
    panic("munmap(), fail to get some args");
  }

  return munmap(va, length);
}