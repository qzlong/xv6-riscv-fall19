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
#define NBUCKETS 13
struct {
  struct spinlock lock[NBUCKETS];
  struct buf buf[NBUF];
  struct spinlock write; //acquired when changing the link or writing into a buf
  // Linked list of all buffers, through prev/next.
  // head.next is most recently used.
  // struct buf head;
  struct buf hashbucket[NBUCKETS];
} bcache;

void
binit(void)
{
  struct buf *b;
  for(int i=0;i<NBUCKETS;++i)
    initlock(&bcache.lock[i], "bcache");
  initlock(&bcache.write,"write");
  // Create linked list of buffers
  for(int i=0;i<NBUCKETS;i++){
    bcache.hashbucket[i].prev = &bcache.hashbucket[i];
    bcache.hashbucket[i].next = &bcache.hashbucket[i];
  }
  for(b = bcache.buf; b < bcache.buf+NBUF; b++){
    initsleeplock(&b->lock, "buffer");
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
  }
  // for(int hashcode = 0;hashcode<NBUCKETS;hashcode++){
  //   bcache.hashbucket[hashcode].prev = bcache.hashbucket[0].prev;
  //   bcache.hashbucket[hashcode].next = bcache.hashbucket[0].next;
  // }
}

// Look through buffer cache for block on device dev.
// If not found, allocate a buffer.
// In either case, return locked buffer.
static struct buf*
bget(uint dev, uint blockno)
{
  struct buf *b;

  int hashcode = blockno % 13;
  acquire(&bcache.lock[hashcode]);
  // Is the block already cached?
  for(b = bcache.hashbucket[0].next; b != &bcache.hashbucket[0]; b = b->next){
    if(b->dev == dev && b->blockno == blockno){
      b->refcnt++;
      release(&bcache.lock[hashcode]);
      acquiresleep(&b->lock);
      return b;
    }
  }
  acquire(&bcache.write);
  // Not cached; recycle an unused buffer.
  for(b = bcache.hashbucket[0].prev; b != &bcache.hashbucket[0]; b = b->prev){
    if(b->refcnt == 0) {
      b->dev = dev;
      b->blockno = blockno;
      b->valid = 0;
      b->refcnt = 1;
      release(&bcache.lock[hashcode]);
      release(&bcache.write);
      acquiresleep(&b->lock);
      return b;
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
    virtio_disk_rw(b->dev, b, 0);
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
  virtio_disk_rw(b->dev, b, 1);
}

// Release a locked buffer.
// Move to the head of the MRU list.
void
brelse(struct buf *b)
{
  if(!holdingsleep(&b->lock))
    panic("brelse");
  int hashcode = b->blockno % 13;
  releasesleep(&b->lock);

  acquire(&bcache.lock[hashcode]);
  b->refcnt--;
  if (b->refcnt == 0) {
    // no one is waiting for it.
    acquire(&bcache.write);
    b->next->prev = b->prev;
    b->prev->next = b->next;
    b->next = bcache.hashbucket[0].next;
    b->prev = &bcache.hashbucket[0];
    bcache.hashbucket[0].next->prev = b;
    bcache.hashbucket[0].next = b;
    release(&bcache.write);
  }
  
  release(&bcache.lock[hashcode]);
}

void
bpin(struct buf *b) {
  int hashcode = b->blockno % 13;
  acquire(&bcache.lock[hashcode]);
  b->refcnt++;
  release(&bcache.lock[hashcode]);
}

void
bunpin(struct buf *b) {
  int hashcode  = b->blockno % 13;
  acquire(&bcache.lock[hashcode]);
  b->refcnt--;
  release(&bcache.lock[hashcode]);
}


