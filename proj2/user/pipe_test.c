#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
int pipe_rt(int[2]);

typedef struct task_t
{
  int task_id;
  int x;
  int y;
  char op;
  int *result;
  int *error;
} task_t;

// Function to perform calculations
int calc(int x, int y, char *op, int *result)
{
  switch (*op)
  {
  case '+':
    *result = x + y;
    break;
  case '-':
    *result = x - y;
    break;
  case '*':
    *result = x * y;
    break;
  case '/':
    if (y == 0)
    {
      return -2;
    }
    *result = x / y;
    break;
  default:
    return -1;
  }
  return 0;
}

// Server function
void server(int clientNums, int read_fd, int write_fd[clientNums][2])
{
  task_t task;
  while (clientNums > 0)
  {
    read(read_fd, &task, sizeof(task));
    int id = task.task_id;
    printf("Parent received task: %d\n", id);
    *task.error = calc(task.x, task.y, &task.op, task.result);
    write(write_fd[id][1], &task, sizeof(task));
    printf("Parent sent processed task: %d\n", id);
    clientNums -= 1;
  }
}

// Client function
void client(int write_fd, int read_fd, task_t task)
{
  write(write_fd, &task, sizeof(task));
  read(read_fd, &task, sizeof(task));
  exit(0);
}

task_t create_task(int i, int *result, int *error)
{
  task_t task;
  task.task_id = i;
  task.x = 10 + i;
  task.y = 2 + i;
  task.result = result;
  task.error = error;
  int opn = i % 4;
  switch (opn)
  {
  case 0:
    task.op = '+';
    break;
  case 1:
    task.op = '-';
    break;
  case 2:
    task.op = '*';
    break;
  case 3:
    task.op = '/';
    break;
  }
  printf("task %d created: %d %s %d \n", i, task.x, &task.op, task.y);
  return task;
}

int main()
{

  int clientNums = 5;
  // Two pipes: p1 for client->server
  // p2 for server->client. p2 needs to == number of clients to receive tasks respectively.
  int p1[2], p2[clientNums][2];
  int result[clientNums];
  int error[clientNums];

  if (pipe(p1) < 0)
  {
    return -1;
  }

  // Fork child processes (clients)
  for (int i = 0; i < clientNums; i++)
  {
    task_t task = create_task(i, &result[i], &error[i]);
    if (pipe(p2[i]) < 0)
    {
      exit(1);
    }

    printf("read fd in %d: %d\n", i, p2[i][0]);
    if (fork() == 0)
    {

      close(p1[0]);    // Close read end of p1 (client writes to p1[1])
      close(p2[i][1]); // Close write end of p2 (client reads from p2[0])

      // Call the client function
      client(p1[1], p2[i][0], task);

      // Exit the child process
      exit(0);
    }
  }

  // Call the server function
  server(clientNums, p1[0], p2);

  // Wait for all child processes to finish
  for (int j = 0; j < clientNums; j += 1)
  {
    int waitpid = wait(0);
    printf("child process: %d terminates \n", waitpid);
    printf("error status: %d, result: %d \n", error[j], result[j]);
  }

  exit(0);
}