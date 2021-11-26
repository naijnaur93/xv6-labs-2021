// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem;

uint8 ref_count[(PHYSTOP - KERNBASE) / PGSIZE];

extern int print_flag;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char*)PGROUNDUP((uint64)pa_start);
  for(; p + PGSIZE <= (char*)pa_end; p += PGSIZE)
    kfree(p);
}

// Free the page of physical memory pointed at by v,
// which normally should have been returned by a
// call to kalloc().  (The exception is when
// initializing the allocator; see kinit above.)
void
kfree(void *pa)
{
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  if (ref_count[((uint64)pa - KERNBASE) / PGSIZE] == 1) {
    // free the page allocated at the booting process
    ref_count[((uint64)pa - KERNBASE) / PGSIZE]--;
  }

  if (ref_count[((uint64)pa - KERNBASE) / PGSIZE] != 0) {
    // printf("[PROMPT] ref_count[%d] = %d, kfree() not performed\n",
    //        ((uint64)pa - KERNBASE) / PGSIZE,
    //        ref_count[((uint64)pa - KERNBASE) / PGSIZE]);
    return;
  }

  if (print_flag) {
    // printf("[PROMPT] freeing page: %d\n",PGIDX(pa));
  }
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  acquire(&kmem.lock);
  r->next = kmem.freelist;
  kmem.freelist = r;
  release(&kmem.lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  acquire(&kmem.lock);
  r = kmem.freelist;
  if(r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    if ((uint64)r < KERNBASE) {
      panic("kalloc, Impossible free space pointer!");
    }
    ref_count[((uint64)r-KERNBASE)/PGSIZE] = 1;  // set the ref_count to 1
    // printf("[PROMPT] ref_count[%d] set to 1 in kalloc()\n",
    //       ((uint64)r - KERNBASE) / PGSIZE);
  }
  return (void*)r;
}
