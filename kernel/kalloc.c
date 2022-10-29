// Physical memory allocator, for user processes,
// kernel stacks, page-table pages,
// and pipe buffers. Allocates whole 4096-byte pages.

#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"

#define NCPUS 3

void freerange(void *pa_start, void *pa_end);
void *kalloc_cpu(int cpu_id);

extern char end[]; // first address after kernel.
                   // defined by kernel.ld.

struct run {
  struct run *next;
};

struct {
  struct spinlock lock[NCPUS];
  struct run *freelist[NCPUS];
} kmem;

uint cpu_size = 0;

void
kinit()
{
  for(int i=0; i<NCPUS; i++){
    initlock(&kmem.lock[i], "kmem");
  }
  cpu_size = ((void*)PHYSTOP - (void*)end) / NCPUS;
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

  uint cpu_num = (pa - (void *)end) / cpu_size;
  acquire(&kmem.lock[cpu_num]);
  r->next = kmem.freelist[cpu_num];
  kmem.freelist[cpu_num] = r;
  release(&kmem.lock[cpu_num]);
}

// Allocate one 4096-byte page of physical memory.
// Returns a pointer that the kernel can use.
// Returns 0 if the memory cannot be allocated.
void *
kalloc(void)
{
  void *r;
  push_off();
  int cpu_id = cpuid();
  pop_off();
  r = kalloc_cpu(cpu_id);
  if(r){
    return r;
  }
  for(int i=0; i<NCPUS; i++){
    if(i == cpu_id){
      continue;
    }
    r = kalloc_cpu(i);
    if(r){
      break;
    }
  }
  return r;
}

void *
kalloc_cpu(int cpu_id){
  acquire(&kmem.lock[cpu_id]);
  struct run *r;
  r = kmem.freelist[cpu_id];
  if(r){
    kmem.freelist[cpu_id] = r->next;
  }
  release(&kmem.lock[cpu_id]);
  if(r){
    memset((char*)r, 5, PGSIZE); // fill with junk
  }
  return (void*)r;
}