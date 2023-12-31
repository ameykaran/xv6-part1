#include "types.h"
#include "param.h"
#include "memlayout.h"
#include "riscv.h"
#include "spinlock.h"
#include "proc.h"
#include "defs.h"
#include "mlfq.h"

struct spinlock tickslock;
uint ticks;

extern char trampoline[], uservec[], userret[];

// in kernelvec.S, calls kerneltrap().
void kernelvec();

extern int devintr();

void trapinit(void)
{
  initlock(&tickslock, "time");
}

// set up to take exceptions and traps while in the kernel.
void trapinithart(void)
{
  w_stvec((uint64)kernelvec);
}

//
// handle an interrupt, exception, or system call from user space.
// called from trampoline.S
//
void usertrap(void)
{
  int which_dev = 0;

  if ((r_sstatus() & SSTATUS_SPP) != 0)
    panic("usertrap: not from user mode");

  // send interrupts and exceptions to kerneltrap(),
  // since we're now in the kernel.
  w_stvec((uint64)kernelvec);

  struct proc *p = myproc();

  // save user program counter.
  p->trapframe->epc = r_sepc();

  if (r_scause() == 8)
  {
    // system call

    if (killed(p))
      exit(-1);

    // sepc points to the ecall instruction,
    // but we want to return to the next instruction.
    p->trapframe->epc += 4;

    // an interrupt will change sepc, scause, and sstatus,
    // so enable only now that we're done with those registers.
    intr_on();

    syscall();
  }
  else if ((which_dev = devintr()) != 0)
  {
    // give up the CPU if this is a timer interrupt.
    if (which_dev == 2)
    {
      p->ticks_elapsed += 1;
      if (p->sigalarm_nticks > 0 && !p->sigalarm_running)
      {
        if (p->ticks_elapsed >= p->sigalarm_nticks)
        {
          p->sigalarm_running = 1;
          p->ticks_elapsed = 0;
          p->trap_backup = (struct trapframe *)kalloc();
          *(p->trap_backup) = *(p->trapframe);
          p->trapframe->epc = p->sigalarm_handler;
        }
      }
#ifndef FCFS
#ifndef MLFQ
      yield();
#endif
#endif

#ifdef MLFQ
      for (struct proc *p = proc; p < &proc[NPROC]; p++)
      {
        if (!p)
          continue;

        acquire(&p->lock);

        // Boost the process
        if ((ticks - p->mlfq_data.intime) >= AGING_TIME && p->mlfq_data.in_queue && p->state == RUNNABLE)
        {
          remove(p, p->mlfq_data.curr_pri);
          int newpri = max(p->mlfq_data.curr_pri - 1, 0);
#ifdef DEBUG
          printf("promoting %d to %d\n", p->pid, newpri);
#endif
          // p->mlfq_data.num_ticks = 0;
          push(p, newpri);

          release(&p->lock);
          continue;
        }

        release(&p->lock);
      }

      if (which_dev == 2 && p && p->state == RUNNING)
      {
        int time;
        switch (p->mlfq_data.curr_pri)
        {
        default:
        case 0:
          time = MLFQ_TICKS0;
          break;
        case 1:
          time = MLFQ_TICKS1;
          break;
        case 2:
          time = MLFQ_TICKS2;
          break;
        case 3:
          time = MLFQ_TICKS3;
          break;
        }

        if (p->mlfq_data.num_ticks >= time)
        {
          int new_pri = min(p->mlfq_data.curr_pri + 1, MLFQ_NUM_QUEUES - 1);
          remove(p, p->mlfq_data.curr_pri);
#ifdef DEBUG
          printf("demoting %d to %d\n", p->pid, new_pri);
#endif
          push(p, new_pri);
          yield();
        }
      }
      int flag = 0;
      for (int i = 0; i < p->mlfq_data.curr_pri; i++)
      {
        for (struct proc *p = proc; p < &proc[NPROC]; p++)
        {
          if (p->state == RUNNABLE && p->mlfq_data.in_queue && p->mlfq_data.curr_pri == i)
          {
            flag = 1;
            break;
          }
        }
        if (flag)
          break;
      }
      if (flag)
        yield();

#endif
    }
  }
  else
  {
    printf("usertrap(): unexpected scause %p pid=%d\n", r_scause(), p->pid);
    printf("            sepc=%p stval=%p\n", r_sepc(), r_stval());
    setkilled(p);
  }

  if (killed(p))
    exit(-1);

#ifndef FCFS
#ifndef MLFQ
  // give up the CPU if this is a timer interrupt.
  if (which_dev == 2)
    yield();
#endif
#endif

  usertrapret();
}

//
// return to user space
//
void usertrapret(void)
{
  struct proc *p = myproc();

  // we're about to switch the destination of traps from
  // kerneltrap() to usertrap(), so turn off interrupts until
  // we're back in user space, where usertrap() is correct.
  intr_off();

  // send syscalls, interrupts, and exceptions to uservec in trampoline.S
  uint64 trampoline_uservec = TRAMPOLINE + (uservec - trampoline);
  w_stvec(trampoline_uservec);

  // set up trapframe values that uservec will need when
  // the process next traps into the kernel.
  p->trapframe->kernel_satp = r_satp();         // kernel page table
  p->trapframe->kernel_sp = p->kstack + PGSIZE; // process's kernel stack
  p->trapframe->kernel_trap = (uint64)usertrap;
  p->trapframe->kernel_hartid = r_tp(); // hartid for cpuid()

  // set up the registers that trampoline.S's sret will use
  // to get to user space.

  // set S Previous Privilege mode to User.
  unsigned long x = r_sstatus();
  x &= ~SSTATUS_SPP; // clear SPP to 0 for user mode
  x |= SSTATUS_SPIE; // enable interrupts in user mode
  w_sstatus(x);

  // set S Exception Program Counter to the saved user pc.
  w_sepc(p->trapframe->epc);

  // tell trampoline.S the user page table to switch to.
  uint64 satp = MAKE_SATP(p->pagetable);

  // jump to userret in trampoline.S at the top of memory, which
  // switches to the user page table, restores user registers,
  // and switches to user mode with sret.
  uint64 trampoline_userret = TRAMPOLINE + (userret - trampoline);
  ((void (*)(uint64))trampoline_userret)(satp);
}

// interrupts and exceptions from kernel code go here via kernelvec,
// on whatever the current kernel stack is.
void kerneltrap()
{
  int which_dev = 0;
  uint64 sepc = r_sepc();
  uint64 sstatus = r_sstatus();
  uint64 scause = r_scause();

  if ((sstatus & SSTATUS_SPP) == 0)
    panic("kerneltrap: not from supervisor mode");
  if (intr_get() != 0)
    panic("kerneltrap: interrupts enabled");

  if ((which_dev = devintr()) == 0)
  {
    printf("scause %p\n", scause);
    printf("sepc=%p stval=%p\n", r_sepc(), r_stval());
    panic("kerneltrap");
  }

// give up the CPU if this is a timer interrupt.
#ifndef FCFS
#ifndef MLFQ
  if (which_dev == 2 && myproc() != 0 && myproc()->state == RUNNING)
    yield();
#endif
#endif

#ifdef MLFQ
  for (struct proc *p = proc; p < &proc[NPROC]; p++)
  {
    if (!p)
      continue;

    acquire(&p->lock);

    // Boost the process
    if ((ticks - p->mlfq_data.intime) >= AGING_TIME && p->mlfq_data.in_queue && p->state == RUNNABLE)
    {
      remove(p, p->mlfq_data.curr_pri);
      int newpri = max(p->mlfq_data.curr_pri - 1, 0);
#ifdef DEBUG
      printf("promoting %d to %d\n", p->pid, newpri);
#endif
      // p->mlfq_data.num_ticks = 0;
      push(p, newpri);

      release(&p->lock);
      continue;
    }
    release(&p->lock);
  }

  struct proc *p = myproc();
  if (!p)
    goto cont;
  if (which_dev == 2 && p && p->state == RUNNING)
  {
    int time;
    switch (p->mlfq_data.curr_pri)
    {
    default:
    case 0:
      time = MLFQ_TICKS0;
      break;
    case 1:
      time = MLFQ_TICKS1;
      break;
    case 2:
      time = MLFQ_TICKS2;
      break;
    case 3:
      time = MLFQ_TICKS3;
      break;
    }

    if (p->mlfq_data.num_ticks >= time)
    {
      int new_pri = min(p->mlfq_data.curr_pri + 1, MLFQ_NUM_QUEUES - 1);
      remove(p, p->mlfq_data.curr_pri);
#ifdef DEBUG
      printf("demoting %d to %d\n", p->pid, new_pri);
#endif
      push(p, new_pri);
      yield();
    }
  }
  int flag = 0;
  for (int i = 0; i < p->mlfq_data.curr_pri; i++)
  {
    for (struct proc *p = proc; p < &proc[NPROC]; p++)
    {
      if (p->state == RUNNABLE && p->mlfq_data.in_queue && p->mlfq_data.curr_pri == i)
      {
        flag = 1;
        break;
      }
    }
    if (flag)
      break;
  }
  if (flag)
    yield();

cont:
#endif
  // the yield() may have caused some traps to occur,
  // so restore trap registers for use by kernelvec.S's sepc instruction.
  w_sepc(sepc);
  w_sstatus(sstatus);
}

void clockintr()
{
  acquire(&tickslock);
  ticks++;
  update_time();
  wakeup(&ticks);
  release(&tickslock);

#ifdef MLFQ_TEST
  print_mlfq();
#endif
}

// check if it's an external interrupt or software interrupt,
// and handle it.
// returns 2 if timer interrupt,
// 1 if other device,
// 0 if not recognized.
int devintr()
{
  uint64 scause = r_scause();

  if ((scause & 0x8000000000000000L) &&
      (scause & 0xff) == 9)
  {
    // this is a supervisor external interrupt, via PLIC.

    // irq indicates which device interrupted.
    int irq = plic_claim();

    if (irq == UART0_IRQ)
    {
      uartintr();
    }
    else if (irq == VIRTIO0_IRQ)
    {
      virtio_disk_intr();
    }
    else if (irq)
    {
      printf("unexpected interrupt irq=%d\n", irq);
    }

    // the PLIC allows each device to raise at most one
    // interrupt at a time; tell the PLIC the device is
    // now allowed to interrupt again.
    if (irq)
      plic_complete(irq);

    return 1;
  }
  else if (scause == 0x8000000000000001L)
  {
    // software interrupt from a machine-mode timer interrupt,
    // forwarded by timervec in kernelvec.S.

    if (cpuid() == 0)
    {
      clockintr();
    }

    // acknowledge the software interrupt by clearing
    // the SSIP bit in sip.
    w_sip(r_sip() & ~2);

    return 2;
  }
  else
  {
    return 0;
  }
}
