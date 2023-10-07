
// #ifdef MLFQ

#include "types.h"
#include "param.h"
#include "spinlock.h"
#include "riscv.h"
#include "defs.h"
#include "mlfq.h"
#include "proc.h"

struct mlfq
{
  struct proc *head[MLFQ_NUM_QUEUES];
  struct proc *tail[MLFQ_NUM_QUEUES];
  int size[MLFQ_NUM_QUEUES];
  int ticks[MLFQ_NUM_QUEUES];
  int agingtime;
};

struct mlfq mlfq;

void initmlfq(void)
{
  for (int i = 0; i < MLFQ_NUM_QUEUES; i++)
  {
    mlfq.size[i] = 0;
    mlfq.head[i] = 0;
    mlfq.tail[i] = 0;
  }

  mlfq.ticks[0] = MLFQ_TICKS0;
  mlfq.ticks[1] = MLFQ_TICKS1;
  mlfq.ticks[2] = MLFQ_TICKS2;
  mlfq.ticks[3] = MLFQ_TICKS3;
  mlfq.agingtime = AGING_TIME;
}

void print_mlfq();

int isempty(int qnum)
{
  return mlfq.size[qnum] == 0;
}

int isfull(int qnum)
{
  return mlfq.size[qnum] == NPROC;
}

int in_queue(int qnum, int pid)
{
  for (int i = 0; i < MLFQ_NUM_QUEUES; i++)
  {
    struct proc *curr = mlfq.head[i];
    while (curr)
    {
      if (curr->pid == pid)
        return 1;
      curr = curr->mlfq_data.next;
    }
  }
  return 0;
}

void enqueue(int qnum, struct proc *p)
{
  if (isfull(qnum))
    panic("Queue is full");
  if (in_queue(qnum, p->pid))
    return;

  printf("Enqueuing %d in qno%d\n", p->pid, qnum);
  p->mlfq_data.curr_pri = qnum;
  p->mlfq_data.num_ticks = 0;
  p->mlfq_data.wtime = 0;

  if (isempty(qnum))
  {
    mlfq.head[qnum] = p;
    mlfq.tail[qnum] = p;
    p->mlfq_data.next = 0;
    p->mlfq_data.prev = 0;
    mlfq.size[qnum] += 1;

    // print_mlfq();

    return;
  }

  struct proc *curr_tail = mlfq.tail[qnum];
  curr_tail->mlfq_data.next = p;
  p->mlfq_data.prev = curr_tail;
  mlfq.tail[qnum] = p;
  mlfq.size[qnum] += 1;
}

void inject(int qnum, struct proc *p)
{
  if (isfull(qnum))
    panic("Queue is full");

  if (in_queue(qnum, p->pid))
    return;

  printf("Injecting %d in qno%d\n", p->pid, qnum);
  p->mlfq_data.curr_pri = qnum;
  p->mlfq_data.num_ticks = 0;
  p->mlfq_data.wtime = 0;

  if (isempty(qnum))
  {
    mlfq.head[qnum] = p;
    mlfq.tail[qnum] = p;
    p->mlfq_data.next = 0;
    p->mlfq_data.prev = 0;
    mlfq.size[qnum] += 1;

    // print_mlfq();

    return;
  }

  struct proc *curr_head = mlfq.head[qnum];
  curr_head->mlfq_data.prev = p;
  p->mlfq_data.next = curr_head;
  mlfq.head[qnum] = p;
  mlfq.size[qnum] += 1;

  // print_mlfq();
}

struct proc *dequeue(int qnum)
{
  if (isempty(qnum))
    panic("Queue is empty");

  struct proc *temp = mlfq.head[qnum];
  while (temp->mlfq_data.next) // && temp->state != RUNNABLE)
    temp = temp->mlfq_data.next;

  if (!temp)
    return 0;

  printf("Dequeuing %d from qno%d\n", temp->pid, temp->mlfq_data.curr_pri);

  struct proc *runnable_proc = temp, *prev, *next;
  prev = runnable_proc->mlfq_data.prev;
  next = runnable_proc->mlfq_data.next;

  if (!prev && !next)
  {
    mlfq.head[qnum] = 0;
    mlfq.tail[qnum] = 0;
  }

  else if (!prev && next)
  {
    next->mlfq_data.prev = 0;
    mlfq.head[qnum] = next;
  }
  else if (prev && !next)
  {
    prev->mlfq_data.next = 0;
    mlfq.tail[qnum] = prev;
  }
  else
  {
    prev->mlfq_data.next = next;
    next->mlfq_data.prev = prev;
  }

  runnable_proc->mlfq_data.num_ticks = 0;
  runnable_proc->mlfq_data.wtime = 0;
  runnable_proc->mlfq_data.prev = 0;
  runnable_proc->mlfq_data.next = 0;

  mlfq.size[qnum] -= 1;

  // print_mlfq();
  return runnable_proc;
}

void remove_proc(struct proc *p, int qnum)
{
  struct proc *curr = mlfq.head[qnum];
  while (curr)
  {
    if (curr == p)
    {
      struct proc *prev = curr->mlfq_data.prev,
                  *next = curr->mlfq_data.next;

      if (prev)
        prev->mlfq_data.next = next;

      if (next)
        next->mlfq_data.prev = prev;

      int arr[MLFQ_NUM_QUEUES];
      memmove(arr, curr->mlfq_data.numran, sizeof(arr));
      memset(&curr->mlfq_data, 0, sizeof(curr->mlfq_data));
      memmove(curr->mlfq_data.numran, arr, sizeof(arr));

      break;
    }
    curr = curr->mlfq_data.next;
  }
}

struct proc *topmost_proc(void)
{
  // printf("Topmost\n");
  // print_mlfq();
  for (int i = 0; i < MLFQ_NUM_QUEUES; i++)
    if (!isempty(i))
    {
      printf("*Q%d is not empty\n", i);
      struct proc *temp = dequeue(i);
      printf("-Q%d is not empty\n", i);
      if (!temp)
        continue;
      printf("top is %d\n", temp->pid);
      return temp;
    }
  // printf("top is %d\n", 0);
  return 0;
};

// void print_mlfq()
// {
//   for (int i = 0; i < MLFQ_NUM_QUEUES; i++)
//     printf("%d - %d; ", i, mlfq.size[i]);
//   printf("\n");

//   for (int i = 0; i < MLFQ_NUM_QUEUES; i++)
//   {
//     if (mlfq.size[i] == 0)
//       continue;
//     printf("Queue num %d: ", i);
//     struct proc *curr = mlfq.head[i];
//     while (curr)
//     {
//       printf("%d(%d:%d,%d) ", curr->pid, curr->state, curr->mlfq_data.prev ? curr->mlfq_data.prev->pid : 0, curr->mlfq_data.next ? curr->mlfq_data.next->pid : 0);
//       curr = curr->mlfq_data.next;
//     }
//     printf("\n");
//   }
// }

// void initial_proc(struct proc *p)
// {
//   // p->mlfq_data.prev = 0;
//   // p->mlfq_data.next = 0;
//   // p->mlfq_data.num_ticks = 0;
//   // p->mlfq_data.wtime = 0;
//   // for (int i = 0; i < MLFQ_NUM_QUEUES; i++)
//   //   p->mlfq_data.numran[i] = 0;

//   // enqueue(0, p);
//   // p->mlfq_data.numran[0] += 1;
//   // // p->mlfq_data.curr_proc = p;
// }

char higher_exists(int qnum)
{
  for (int i = 0; i < qnum; i++)
    if (mlfq.size[i])
      return 1;
  return 0;
}

void mlfq_upgrade(int which_dev)
{
  // printf("mlfq update called\n");
  struct proc *p = myproc();

  if (!p)
    return;

  char higher_bool = higher_exists(p->mlfq_data.curr_pri);
  if (higher_bool)
  {
    enqueue(p->mlfq_data.curr_pri, p);
    yield();
  }

  if (which_dev == 2 && p && p->state == RUNNING && p->mlfq_data.num_ticks > mlfq.ticks[p->mlfq_data.curr_pri])
  {
    remove_proc(p, p->mlfq_data.curr_pri);
    int newpri = min(p->mlfq_data.curr_pri + 1, 0);
    printf("demoting %d to %d\n", p->pid, newpri);
    p->mlfq_data.curr_pri = newpri;
    enqueue(newpri, p);
    yield();
  }

  // printf("**\n");
}

// #endif
