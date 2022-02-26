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

#define NBKT 17
#define BKTSIZE 17

struct bkt{
  struct spinlock lock;
  struct buf buf[BKTSIZE];
};
struct {
  struct spinlock lock;
  struct bkt bkt[NBKT];
} bcache;

void
binit(void)
{
  struct buf *b;
  initlock(&bcache.lock, "bcache");
  for(int i = 0; i < NBKT; ++i)
    initlock(&bcache.bkt[i].lock, "bcache.bkt");

  for(int i = 0; i < NBKT; ++i)
    for(b = bcache.bkt[i].buf; b < bcache.bkt[i].buf+BKTSIZE; ++b) {
      initsleeplock(&b->lock, "buffer");
      b->ts = 0; // init timestamp
      b->refcnt = 0;
      b->valid = 0;
      b->disk = 0;
    }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  int min = 0;
  int slot = blockno % NBKT;
  struct buf *b, *end = bcache.bkt[slot].buf + BKTSIZE;

  acquire(&bcache.bkt[slot].lock);

  // Is the block already cached?
  for(b = bcache.bkt[slot].buf; b < end; ++b) {
    if(b->dev == dev && b->blockno == blockno) {
      ++b->refcnt;
      release(&bcache.bkt[slot].lock);
      acquiresleep(&b->lock);
      return b;
    }
  }

  // Not cached.
  // Recycle the least recently used (LRU) unused buffer.
  b = bcache.bkt[slot].buf;
  for(int i = 0; i < BKTSIZE; ++i) {
    if(b[i].refcnt == 0 && b[i].ts < b[min].ts)
      min = i;
  }
  b = b + min;
  if(b->refcnt == 0) {
    b->dev = dev;
    b->blockno = blockno;
    b->valid = 0;
    b->refcnt = 1;
    release(&bcache.bkt[slot].lock);
    acquiresleep(&b->lock);
    return b;
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
  int slot = b->blockno % NBKT;
  if(!holdingsleep(&b->lock))
    panic("brelse");

  releasesleep(&b->lock);
  acquire(&bcache.bkt[slot].lock);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    b->ts = ticks;
  }
  release(&bcache.bkt[slot].lock);
}

void
bpin(struct buf *b) {
  int slot = b->blockno % NBKT;
  acquire(&bcache.bkt[slot].lock);
  b->refcnt++;
  release(&bcache.bkt[slot].lock);
}

void
bunpin(struct buf *b) {
  int slot = b->blockno % NBKT;
  acquire(&bcache.bkt[slot].lock);
  b->refcnt--;
  release(&bcache.bkt[slot].lock);
}


