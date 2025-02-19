#define MAX_PRIORITY 0x7FFFFFFF // Maximum priority value

// Matches user-space task structure
struct task_t
{
  int priority;
  int x;
  int y;
  char op;
  int result;
  int error;
};

// Priority queue node
struct task_node
{
  struct task_t task;
  struct task_node *next;
};

// Priority pipe structure
struct priority_pipe
{
  struct spinlock lock;
  struct task_node *head;   // Highest priority task first
  int readopen;             // Read FD active
  int writeopen;            // Write FD active
  struct proc *wchan_read;  // Sleep channel for readers
  struct proc *wchan_write; // Sleep channel for writers
};
