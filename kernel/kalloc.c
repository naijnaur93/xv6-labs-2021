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

struct RefCount
{
  struct spinlock lock;
  uint8 ref_count[(PHYSTOP - KERNBASE) / PGSIZE];
} ref_count;

extern int print_flag;

void decrease_ref(uint64 pa) {
  acquire(&ref_count.lock);
  if (ref_count.ref_count[PGIDX(pa)] > 0) {
    ref_count.ref_count[PGIDX(pa)]--;
  } else if (print_flag) {
    // printf("[WARN] Try to decrease a zero count page @%d\n", PGIDX(pa));
  }
  release(&ref_count.lock);
}

void increase_ref(uint64 pa) {
  acquire(&ref_count.lock);
  ref_count.ref_count[PGIDX(pa)]++;
  release(&ref_count.lock);
}

uint8 get_ref_count(uint64 pa) {
  uint8 ret;
  acquire(&ref_count.lock);
  ret = ref_count.ref_count[PGIDX(pa)];
  release(&ref_count.lock);
  return ret;
}

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  freerange(end, (void*)PHYSTOP);

  initlock(&ref_count.lock, "ref_count");
  for (int i = 0; i < (PHYSTOP - KERNBASE) / PGSIZE; i++) {
    acquire(&ref_count.lock);
    ref_count.ref_count[i] = 0;
    release(&ref_count.lock);
  }
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
  acquire(&kmem.lock);
  struct run *r;

  if(((uint64)pa % PGSIZE) != 0 || (char*)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  decrease_ref((uint64)pa);

  if (get_ref_count((uint64)pa) != 0) {
    // printf("[PROMPT] ref_count[%d] = %d, kfree() not performed\n",
    //        PGIDX(pa),
    //        get_ref_count((uint64)pa));
    release(&kmem.lock);
    return;
  }

  if (print_flag) {
    // printf("[PROMPT] freeing page: %d\n",PGIDX(pa));
  }
  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

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

  if(r) {
    memset((char*)r, 5, PGSIZE); // fill with junk
    if ((uint64)r < KERNBASE) {
      panic("kalloc, Impossible free space pointer!");
    }
    increase_ref((uint64)r);  // set the ref_count to 1
    // check sanity
    if (get_ref_count((uint64)r) != 1) {
      printf("[ERROR] kalloc(), ref_count[%d] = %d\n", PGIDX(r),
             get_ref_count((uint64)r));
      // return r to free list
      r->next = kmem.freelist;
      kmem.freelist = r;
      release(&kmem.lock);
      return 0;
    }
    // printf("[PROMPT] ref_count[%d] set to 1 in kalloc()\n",
    //       ((uint64)r - KERNBASE) / PGSIZE);
  }
  release(&kmem.lock);
  return (void*)r;
}
