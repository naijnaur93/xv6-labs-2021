// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "proc.h"

void freerange(void *pa_start, void *pa_end);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmem[NCPU];


void
kinit()
{
  int i;
  for (i = 0; i < NCPU; i++) {
    initlock(&kmem[i].lock, "kmem");
  }
  // turn off interrupts to secure cpuid()
  push_off();
  // give all free memory to the CPU running freerange()
  acquire(&kmem[cpuid()].lock);
  freerange(end, (void *)PHYSTOP);
  release(&kmem[cpuid()].lock);
  // restore interrupt status
  pop_off();
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;
  push_off();
  r->next = kmem[cpuid()].freelist;
  kmem[cpuid()].freelist = r;
  pop_off();
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;

  push_off();
  acquire(&kmem[cpuid()].lock);
  r = kmem[cpuid()].freelist;
  if(r) {
    kmem[cpuid()].freelist = r->next;
  } else {
    // steal memory from other CPUs
    // acquire its own memory lock to avoid potential loss of free page
    int i;
    for (i = 0; i < NCPU; i++) {
      if (i != cpuid()) {
        acquire(&kmem[i].lock);
        r = kmem[i].freelist;
        if (r) {  // available memory found
          kmem[i].freelist = r->next;
          release(&kmem[i].lock);
          release(&kmem[cpuid()].lock);
          break;
      }
      release(&kmem[i].lock);
      }
    }
  }
  // remember to release `kmem[cpuid()].lock` if there's no memory can be stolen
  if (holding(&kmem[cpuid()].lock)) release(&kmem[cpuid()].lock);
  pop_off();

  if (r) memset((char *)r, 5, PGSIZE);  // fill with junk
  return (void *)r;
}
