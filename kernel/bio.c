// Buffer cache.
//
// The buffer cache is a linked list of buf structures holding
// cached copies of disk block contents.  Caching disk blocks
// in memory reduces the number of disk reads and also provides
// a synchronization point for disk blocks used by multiple processes.
//
// Interface:
// * To get a buffer for a particular disk block, call bread.
// * After changing buffer data, call bwrite to write it to disk.
// * When done with the buffer, call brelse.
// * Do not use the buffer after calling brelse.
// * Only one process at a time can use a buffer,
//     so do not keep them longer than necessary.


#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "sleeplock.h"
#include "riscv.h"
#include "defs.h"
#include "fs.h"
#include "buf.h"

#define HASHSZ 13

struct bucket {
  struct buf head;
  struct spinlock lk;
};

struct {
  struct spinlock lock;
  struct buf buf[NBUF];

  // Linked list of all buffers, through prev/next.
  // Sorted by how recently the buffer was used.
  // head.next is most recent, head.prev is least.
  struct bucket table[HASHSZ];
} bcache;

void
binit(void)
{
  struct buf *b;

  initlock(&bcache.lock, "bcache");

  // Create linked list of buffers
  int i;
  for (i = 0; i < HASHSZ; i++) {
    initlock(&bcache.table[i].lk, "bucket");
    bcache.table[i].head.prev = &bcache.table[i].head;
    bcache.table[i].head.next = &bcache.table[i].head;
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    b->next = bcache.table[0].head.next;
    b->prev = &bcache.table[0].head;
    initsleeplock(&b->lock, "buffer");
    bcache.table[0].head.next->prev = b;
    bcache.table[0].head.next = b;
  }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;
  int bckt_idx = blockno % HASHSZ;

  acquire(&bcache.lock);

  // printf("bget(blockno = %d), hash to bucket %d\n", blockno, bckt_idx);
  // Is the block already cached?
  for(b = bcache.table[bckt_idx].head.next; b != &bcache.table[bckt_idx].head; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      // printf("find CACHED blockno %d in table[%d]\n", blockno, bckt_idx);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }
  // printf("block %d not cached, find free block in the bucket %d\n", blockno,
  //        bckt_idx);
  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  for(b = bcache.table[bckt_idx].head.prev; b != &bcache.table[bckt_idx].head; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      // printf("find FREE blockno %d in table[%d]\n", blockno, bckt_idx);
      release(&bcache.lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // steal from other buckets
  int i;
  for (i = 0; i < HASHSZ; i++) {
    if (i != bckt_idx) {
      // printf("current bucket %d, search in bucket %d\n", bckt_idx, i);
      for (b = bcache.table[i].head.prev; b != &bcache.table[i].head;
           b = b->prev) {
        if (b->refcnt == 0) {
          b->dev = dev;
          b->blockno = blockno;
          b->valid = 0;
          b->refcnt = 1;
          // printf("find FREE blockno %d in table[%d]\n", blockno, bckt_idx);
          // remove b from its original bucket
          b->next->prev = b->prev;
          b->prev->next = b->next;
          // add b to the head of the current bucket
          b->next = bcache.table[bckt_idx].head.next;
          b->prev = &bcache.table[bckt_idx].head;
          bcache.table[bckt_idx].head.next->prev = b;
          bcache.table[bckt_idx].head.next = b;

          release(&bcache.lock);
          acquiresleep(&b->lock);
          return b;
        }
      }
    }
  }
  panic("bget: no buffers");
}

// Return a locked buf with the contents of the indicated block.
struct buf*
bread(uint dev, uint blockno)
{
  struct buf *b;

  b = bget(dev, blockno);
  if(!b->valid) {
    virtio_disk_rw(b, 0);
    b->valid = 1;
  }
  return b;
}

// Write b's contents to disk.  Must be locked.
void
bwrite(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("bwrite");
  virtio_disk_rw(b, 1);
}

// Release a locked buffer.
// Move to the head of the most-recently-used list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);

  acquire(&bcache.lock);
  b->refcnt--;
  int hash_idx = b->blockno % HASHSZ;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.table[hash_idx].head.next;
    b->prev = &bcache.table[hash_idx].head;
    bcache.table[hash_idx].head.next->prev = b;
    bcache.table[hash_idx].head.next = b;
  }

  release(&bcache.lock);
}

void
bpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt++;
  release(&bcache.lock);
}

void
bunpin(struct buf *b) {
  acquire(&bcache.lock);
  b->refcnt--;
  release(&bcache.lock);
}


