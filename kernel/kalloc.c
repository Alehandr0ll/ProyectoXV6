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

// Requerimiento B: contador de referencias por página física.
// Con Copy-on-Write varios procesos pueden apuntar a la misma página,
// así que solo se devuelve al freelist cuando el contador llega a 0.
// Hay una entrada por cada página entre KERNBASE y PHYSTOP.
#define PA2IDX(pa) (((uint64)(pa) - KERNBASE) / PGSIZE)

struct {
  struct spinlock lock; // protege count[] de actualizaciones concurrentes
  int count[(PHYSTOP - KERNBASE) / PGSIZE];
} kref;

void
kinit()
{
  initlock(&kmem.lock, "kmem");
  initlock(&kref.lock, "kref");
  freerange(end, (void *)PHYSTOP);
}

void
freerange(void *pa_start, void *pa_end)
{
  char *p;
  p = (char *)PGROUNDUP((uint64)pa_start);
  for (; p + PGSIZE <= (char *)pa_end; p += PGSIZE) {
    // kfree() descuenta una referencia; se parte en 1 para que
    // cada página quede en 0 y entre al freelist.
    kref.count[PA2IDX(p)] = 1;
    kfree(p);
  }
}

// Drop one reference to the page of physical memory pointed at by pa,
// which normally should have been returned by a call to kalloc().
// The page is returned to the free list only when its reference count
// reaches zero. (The exception is when initializing the allocator;
// see kinit above.)
void
kfree(void *pa)
{
  struct run *r;
  int refs;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("kfree");

  acquire(&kref.lock);
  if (kref.count[PA2IDX(pa)] < 1)
    panic("kfree: refcount");
  refs = --kref.count[PA2IDX(pa)];
  release(&kref.lock);

  // Otros procesos aún usan la página: no se libera.
  if (refs > 0)
    return;

  // Fill with junk to catch dangling refs.
  memset(pa, 1, PGSIZE);

  r = (struct run *)pa;

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
  if (r)
    kmem.freelist = r->next;
  release(&kmem.lock);

  if (r) {
    // Una página recién asignada tiene un único dueño.
    acquire(&kref.lock);
    if (kref.count[PA2IDX(r)] != 0)
      panic("kalloc: refcount");
    kref.count[PA2IDX(r)] = 1;
    release(&kref.lock);

    memset((char *)r, 5, PGSIZE); // fill with junk
  }
  return (void *)r;
}

// Agrega una referencia a la página física pa, que debe estar en uso.
// Se llama cuando otro proceso pasa a compartirla (fork con COW).
void
krefinc(void *pa)
{
  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("krefinc");

  acquire(&kref.lock);
  if (kref.count[PA2IDX(pa)] < 1)
    panic("krefinc: refcount");
  kref.count[PA2IDX(pa)]++;
  release(&kref.lock);
}

// Retorna cuántas referencias tiene la página física pa.
int
krefcount(void *pa)
{
  int n;

  if (((uint64)pa % PGSIZE) != 0 || (char *)pa < end || (uint64)pa >= PHYSTOP)
    panic("krefcount");

  acquire(&kref.lock);
  n = kref.count[PA2IDX(pa)];
  release(&kref.lock);
  return n;
}
