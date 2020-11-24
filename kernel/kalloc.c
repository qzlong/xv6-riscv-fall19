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
struct run* alloc(int cpuid);
int getCPUId();
extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock;
  struct run *freelist;
} kmems[NCPU];

void
kinit()
{
  // int CURCPU = getCPUId();
  for(int i=0;i<NCPU;++i)
    initlock(&kmems[i].lock, "kmem");
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

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run*)pa;

  int CURCPU = getCPUId();
  // printf("%d",CURCPU);
  acquire(&kmems[CURCPU].lock);
  r->next = kmems[CURCPU].freelist;
  kmems[CURCPU].freelist = r;
  release(&kmems[CURCPU].lock);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  struct run *r;
  
  // acquire(&kmems[CURCPU].lock);
  // r = kmems[CURCPU].freelist;
  // if(r)
  //   kmems[CURCPU].freelist = r->next;
  // release(&kmem[CURCPU].lock);

  int CURCPU = getCPUId();
  r = alloc(CURCPU);
  // printf("%d",CURCPU);
  if(r) //get page from current cpu's freelist
    memset((char*)r, 5, PGSIZE); // fill with junk

  else{ //get the page from other cpus' freelist
    for(int i=0;i<NCPU;i++){
      r = alloc(i);
      if(r){
        memset((char*)r, 5, PGSIZE);
        break;
      }
    }
  }
  return (void*)r;
}

int getCPUId(){
  push_off();
  int res = cpuid();
  pop_off();
  return res;
}

struct run* alloc(int cpuid){
  struct run* r;
  acquire(&kmems[cpuid].lock);
  r = kmems[cpuid].freelist;
  if(r)
    kmems[cpuid].freelist = r->next;
  release(&kmems[cpuid].lock);
  return r;
}