#include "types.h"
#include "riscv.h"
#include "defs.h"
#include "param.h"
#include "spinlock.h"
#include "proc.h"
#include "syscall.h"

struct readcount
{
  struct spinlock lock;
  int count;
};

struct readcount readcount = {0};

void increment_read_count(int num)
{
  if (num == SYS_read)
  {
    acquire(&readcount.lock);
    readcount.count += 1;
    release(&readcount.lock);
  }
}

int readcountfunc(void)
{
  int count = -1;
  acquire(&readcount.lock);
  count = readcount.count;
  release(&readcount.lock);
  return count;
}

void readcountinit(void)
{
  initlock(&readcount.lock, "readcount");
  readcount.count = 0;
}