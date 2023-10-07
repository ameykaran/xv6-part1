
// #ifdef MLFQ

// void initial_proc(struct proc *p);
// void mlfq_schedulenext(struct cpu *c, struct proc *currproc)

// MLFQ constants
#define MLFQ_NUM_QUEUES 4
#define MLFQ_TICKS0 1
#define MLFQ_TICKS1 3
#define MLFQ_TICKS2 9
#define MLFQ_TICKS3 15
#define AGING_TIME 30

#define max(a, b) ((a) > (b) ? a : b);
#define min(a, b) ((a) < (b) ? a : b);

struct proc *topmost_proc(void);
void inject(int qnum, struct proc *p);
void enqueue(int qnum, struct proc *p);
struct proc *dequeue(int qnum);
void remove_proc(struct proc *p, int qnum);
void mlfq_upgrade(int which_dev);

// void print_mlfq();

// #endif