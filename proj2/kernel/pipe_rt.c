#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "kernel/syscall.h"
#include "kernel/defs.h"
#include "kernel/file.h"

#include "priority_queue.h"

struct pipe_rt
{
  struct spinlock lock;
  priority_queue_t pq; // Priority queue for tasks
  int nwrite;          // Number of writes
  int nread;           // Number of reads
  int task_id_counter; // Counter for generating unique task IDs
  int readopen;        // read fd is still open
  int writeopen;       // write fd is still open
};

int pipealloc_rt(struct file **f0, struct file **f1)
{
  struct pipe_rt *pi;

  pi = 0;
  *f0 = *f1 = 0;

  /* Allocate file descriptor */
  if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;

  /* Allocate pipe_rt structure from kalloc */
  if ((pi = (struct pipe_rt *)kalloc()) == 0)
    goto bad;

  /* Initialize the pipe_rt structure */
  initlock(&pi->lock, "pipe_rt");

  /* Initialize priority queue */
  pq_init(&pi->pq);

  pi->task_id_counter = 0;
  pi->readopen = 1;
  pi->writeopen = 1;

  /* Set up the file descriptors: read end and write end */
  (*f0)->type = FD_PIPERT;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe = (struct pipe *)pi;

  (*f1)->type = FD_PIPERT;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe = (struct pipe *)pi;

  return 0;

bad:
  if (pi)
    kfree((char *)pi);
  if (*f0)
    fileclose(*f0);
  if (*f1)
    fileclose(*f1);
  return -1;
}

void pipeclose_rt(struct pipe_rt *pi, int writable)
{
  acquire(&pi->lock);
  if (writable)
  {
    pi->writeopen = 0;
    wakeup((void *)&pi->nread); // Wake up readers since no more writes coming
  }
  else
  {
    pi->readopen = 0;
  }

  /* If both ends are closed */
  if (pi->readopen == 0 && pi->writeopen == 0)
  {
    release(&pi->lock);
    kfree((char *)pi);
  }
  else
    release(&pi->lock);
}

int pipewrite_rt(struct pipe_rt *pi, uint64 addr, int n)
{
  struct task_rt task;

  /* Check if write size matches task_rt size */
  if (n != sizeof(struct task_rt))
    return -1;

  /* Copy task from user space */
  if (copyin(myproc()->pagetable, (char *)&task, addr, sizeof(task_rt)) < 0)
    return -1;

  acquire(&pi->lock);

  /* Insert task into priority queue */
  if (pq_insert(&pi->pq, &task) < 0)
  {
    release(&pi->lock);
    return -1;
  }

  pi->nwrite++;

  /* Wake up any sleeping readers */
  wakeup((void *)&pi->nread);

  release(&pi->lock);
  return sizeof(struct task_rt);
}

int piperead_rt(struct pipe_rt *pi, uint64 addr, int n)
{
  struct task_rt task;
  struct proc *pr = myproc();

  /* Check if read size matches task_rt size */
  if (n != sizeof(struct task_rt))
    return -1;

  acquire(&pi->lock);

  /* Wait until there's data to read */
  while (pi->nwrite == pi->nread && pi->writeopen)
  {
    if (killed(pr))
    {
      release(&pi->lock);
      return -1;
    }
    sleep((void *)&pi->nread, &pi->lock);
  }

  /* Try to pop highest priority task */
  if (pq_pop(&pi->pq, &task) < 0)
  {
    release(&pi->lock);
    return -1;
  }

  pi->nread++;

  /* Copy task to user space */
  if (copyout(myproc()->pagetable, addr, (char *)&task, sizeof(task_rt)) < 0)
  {
    release(&pi->lock);
    return -1;
  }

  release(&pi->lock);
  return sizeof(struct task_rt);
}
