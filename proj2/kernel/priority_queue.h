#define PQ_MAX_NODES 64 // Adjust as needed

typedef struct task_rt
{
    int priority;
    int x;
    int y;
    char op; // Supports "+", "-", "*", "/"
    int *result;
    int *error;
    int task_id;
} task_rt;

typedef struct pq_node_t
{
    task_rt data;
    int priority;
    struct pq_node_t *next;
} pq_node_t;

typedef struct
{
    pq_node_t *head;
    pq_node_t nodes[PQ_MAX_NODES]; // Static memory pool
    pq_node_t *free_list;          // Free list for available nodes
} priority_queue_t;

void pq_init(priority_queue_t *pq);
int pq_insert(priority_queue_t *pq, task_rt *src);
int pq_pop(priority_queue_t *pq, task_rt *dst);