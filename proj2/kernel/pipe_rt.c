#include "kernel/types.h"
#include "kernel/param.h"
#include "kernel/memlayout.h"
#include "kernel/riscv.h"
#include "kernel/spinlock.h"
#include "kernel/proc.h"
#include "kernel/syscall.h"
#include "kernel/defs.h"
#include "kernel/fs.h"
#include "kernel/sleeplock.h"
#include "kernel/file.h"
#define PIPESIZE 512
typedef struct task_t
{
  int priority;
  int x;
  int y;
  char op;     // Supports "+", "-", "*", "/"
  int *result; // Stores the computation result
  int *error;  // Returns error if the input is invalid
} task_t;

struct pipe_rt
{
  struct spinlock lock;
  task_t tasks[PIPESIZE];
  uint64 nread;     // number of tasks read
  uint64 nwrite;    // number of tasks written
  uint64 readopen;  // read fd is still open
  uint64 writeopen; // write fd is still open
};

int pipe_rt_alloc(struct file **f0, struct file **f1)
{
  struct pipe_rt *pi;

  pi = 0;
  *f0 = *f1 = 0;
  if ((*f0 = filealloc()) == 0 || (*f1 = filealloc()) == 0)
    goto bad;
  if ((pi = (struct pipe_rt *)kalloc()) == 0)
    goto bad;
  pi->readopen = 1;
  pi->writeopen = 1;
  pi->nwrite = 0;
  pi->nread = 0;
  initlock(&pi->lock, "pipe_rt");
  (*f0)->type = FD_PIPE;
  (*f0)->readable = 1;
  (*f0)->writable = 0;
  (*f0)->pipe_rt = pi;
  (*f1)->type = FD_PIPE;
  (*f1)->readable = 0;
  (*f1)->writable = 1;
  (*f1)->pipe_rt = pi;
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

void pipe_rt_close(struct pipe_rt *pi, int writable)
{
  acquire(&pi->lock);
  if (writable)
  {
    pi->writeopen = 0;
    wakeup(&pi->nread);
  }
  else
  {
    pi->readopen = 0;
    wakeup(&pi->nwrite);
  }
  if (pi->readopen == 0 && pi->writeopen == 0)
  {
    release(&pi->lock);
    kfree((char *)pi);
  }
  else
    release(&pi->lock);
}

int pipe_rt_write(struct pipe_rt *pi, uint64 addr, int n)
{
  struct proc *pr = myproc();
  task_t new_task;

  if (n != sizeof(task_t))
    return -1;

  acquire(&pi->lock);

  // Copy task from user space
  if (copyin(pr->pagetable, (char *)&new_task, addr, sizeof(task_t)) == -1)
  {
    release(&pi->lock);
    return -1;
  }

  // Find correct position based on priority
  int insert_pos = pi->nwrite;
  for (int i = pi->nread; i < pi->nwrite; i++)
  {
    if (pi->tasks[i % PIPESIZE].priority < new_task.priority)
    {
      insert_pos = i;
      break;
    }
  }

  // Shift tasks to make room for new task
  if (insert_pos < pi->nwrite)
  {
    for (int i = pi->nwrite; i > insert_pos; i--)
    {
      pi->tasks[i % PIPESIZE] = pi->tasks[(i - 1) % PIPESIZE];
    }
  }

  // Insert new task
  pi->tasks[insert_pos % PIPESIZE] = new_task;
  pi->nwrite++;

  wakeup(&pi->nread);
  release(&pi->lock);
  return sizeof(task_t);
}

int pipe_rt_read(struct pipe_rt *pi, uint64 addr, int n)
{
  struct proc *pr = myproc();

  acquire(&pi->lock);
  while (pi->nread == pi->nwrite && pi->writeopen)
  {
    if (killed(pr))
    {
      release(&pi->lock);
      return -1;
    }
    sleep(&pi->nread, &pi->lock);
  }

  if (pi->nread < pi->nwrite)
  {
    if (copyout(pr->pagetable, addr, (char *)&pi->tasks[pi->nread % PIPESIZE], sizeof(task_t)) == -1)
    {
      release(&pi->lock);
      return -1;
    }
    pi->nread++;
    wakeup(&pi->nwrite);
    release(&pi->lock);
    return sizeof(task_t);
  }

  release(&pi->lock);
  return 0;
}
static int
fdalloc(struct file *f)
{
  int fd;
  struct proc *p = myproc();

  for (fd = 0; fd < NOFILE; fd++)
  {
    if (p->ofile[fd] == 0)
    {
      p->ofile[fd] = f;
      return fd;
    }
  }
  return -1;
}
uint64 sys_pipe_rt(void)
{
  uint64 fdarray;
  struct file *rf, *wf;
  int fd0, fd1;
  struct proc *p = myproc();
  argaddr(0, &fdarray);

  if (pipe_rt_alloc(&rf, &wf) < 0)
    return -1;
  fd0 = -1;
  if ((fd0 = fdalloc(rf)) < 0 || (fd1 = fdalloc(wf)) < 0)
  {
    if (fd0 >= 0)
      p->ofile[fd0] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  if (copyout(p->pagetable, fdarray, (char *)&fd0, sizeof(fd0)) < 0 ||
      copyout(p->pagetable, fdarray + sizeof(fd0), (char *)&fd1, sizeof(fd1)) < 0)
  {
    p->ofile[fd0] = 0;
    p->ofile[fd1] = 0;
    fileclose(rf);
    fileclose(wf);
    return -1;
  }
  return 0;
}
