#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"

typedef struct task_p {
    int priority;
    int x;
    int y;
    char op;  // Supports "+", "-", "*", "/"
    int *result;
    int *error;
    int task_id;
}  task_p;

int calc(int x, int y, char* op, int *result) {
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
        if (y == 0) {
            return -2;
        }
        *result = x / y;
        break;
    default:
        return -1;
    }

    return 0;
}

void server(int clients, int read_fd, int write_fd) {
    task_p task;
    while (clients > 0) {
        /* read from pipe */
        read(read_fd, &task, sizeof(task));
        int id = task.task_id;
        printf("Parent received task: %d\n", id);

        /* do the calculation */
        *task.error = calc(task.x, task.y, &(task.op), task.result);
        /* write tasks to corresponding pipe */
        write(write_fd, &task, sizeof(task));
        printf("Parent sent processed task: %d\n", id);

        clients -= 1;
    }
}

void client(int write_fd, int read_fd, task_p task) {
    /* send task to pipe */
    write(write_fd, &task, sizeof(task));
    printf("Child sent task: %d\n", task.task_id);
    printf("x: %d, y: %d, op: %c\n", task.x, task.y, task.op);

    /* read the result from output pipe */
    read(read_fd, &task, sizeof(task));
    printf("Child received result: %d\n", task.task_id);
    if (*task.error == -1) {
        printf("task has invalid operator\n");
    } else if (*task.error == -2) {
        printf("task has division by zero exception\n");
    } else {
        printf("result: %d\n",  *task.result);
    }

    exit(0);
}

task_p create_task_p(int generator, int* result, int* error) {
    task_p t;
    t.x = generator % 10;
    t.y = (10 - generator) % 10;
    t.task_id = generator;
    t.result = result;
    t.error = error;
    int opn = generator % 4;
    switch (opn) {
        case 0: t.op = '+';
            break;
        case 1: t.op = '-';
            break;
        case 2: t.op = '*';
            break;
        case 3: t.op = '/';
            break;
    }
    printf("task %d created: %d %s %d \n", generator, t.x, &t.op, t.y);
    return t;
}

int main() {
    /* test user program for prioritized pipe */
    int num_tasks = 10;
    int priorities[] = {5, 7, 3, 2, 9, 0, 1, 6, 8, 4};
    int input_fd[2];
    int output_fd[2];
    int result[num_tasks];
    int error[num_tasks];

    /* syscall to init pipe_rt and pipe */
    if (pipe_rt(input_fd) < 0) {
        return -1;
    }

    if (pipe(output_fd) < 0) {
        return -1;
    }

    /* in this test case only one child process is used to deliver tasks */
    if (fork() == 0) {
        for (int i = 0; i < num_tasks; i += 1) {
            /* create task based on task id */
            task_p task = create_task_p(i, &result[i], &error[i]);
            task.priority = priorities[i];

            /* deliver the task to parent process and receive processed task */
            client(input_fd[1], output_fd[0], task);
        }
    }

    /* server */
    server(num_tasks, input_fd[0], output_fd[1]);
    
    /* wait for child process to terminate */
    wait(0);
    exit(0);
}