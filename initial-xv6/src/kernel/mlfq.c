#ifdef MLFQ

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "mlfq.h"
#include "proc.h"

void print_mlfq();

void push(struct proc *p, int qnum)
{
  p->mlfq_data.curr_pri = qnum;
  p->mlfq_data.in_queue = 1;
  p->mlfq_data.intime = ticks;
  p->mlfq_data.wtime = 0;
  p->mlfq_data.num_ticks = 0;
}

void remove(struct proc *p, int qnum)
{
  p->mlfq_data.in_queue = 0;
  p->mlfq_data.wtime = 0;
  p->mlfq_data.num_ticks = 0;
  p->mlfq_data.intime = 0;
}

struct proc *topmost(int qnum)
{
  struct proc *first = 0;
  for (struct proc *p = proc; p < &proc[NPROC]; p++)
  {
    if (p->mlfq_data.in_queue && p->mlfq_data.curr_pri == qnum && p->state == RUNNABLE)
    {
      if (!first || first->mlfq_data.intime > p->mlfq_data.intime)
        first = p;
    }
  }
  return first;
}

void print_mlfq()
{
  int ct = 0;
  for (struct proc *p = proc; p < &proc[NPROC]; p++)
  {
    if (p->state == UNUSED || p->pid < 4)
      continue;
    if (p->mlfq_data.in_queue)
      ct++;
  }
  if (ct)
  {
    printf("%d ", ticks);
    for (struct proc *p = proc; p < &proc[NPROC]; p++)
    {
      if (p->state == UNUSED || p->pid < 4)
        continue;
      if (p->mlfq_data.in_queue)
        printf("%d(%d,%d) ", p->pid, p->mlfq_data.curr_pri, p->state);
    }
    printf("\n");
  }
}
#endif