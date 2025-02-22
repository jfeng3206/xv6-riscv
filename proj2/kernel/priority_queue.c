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

/* Initialize the priority queue and set up the free list */
void pq_init(priority_queue_t *pq) {
    // initlock(&pq->lock, "priority_queue");
    pq->head = 0;
    pq->free_list = 0;

    // Initialize free list with preallocated nodes
    for (int i = 0; i < PQ_MAX_NODES - 1; i++) {
        pq->nodes[i].next = &pq->nodes[i + 1];
    }
    pq->nodes[PQ_MAX_NODES - 1].next = 0;
    pq->free_list = &pq->nodes[0];  // First free node
}

/* Allocate a node from the static pool */
static pq_node_t* pq_alloc_node(priority_queue_t *pq) {
    if (!pq->free_list) return 0; // No free nodes available
    pq_node_t *node = pq->free_list;
    pq->free_list = pq->free_list->next;
    return node;
}

/* Free a node back to the static pool */
static void pq_free_node(priority_queue_t *pq, pq_node_t *node) {
    node->next = pq->free_list;
    pq->free_list = node;
}

/* Insert a new item into the priority queue */
int
pq_insert(priority_queue_t *pq, task_rt* src) {
    // acquire(&pq->lock);
    
    /* allocate space for a node from free list */
    pq_node_t *new_node = pq_alloc_node(pq);

    if (!new_node) {
        /* no extra space */
    //   release(&pq->lock);
        return -1;
    }

    memmove(&(new_node->data), src, sizeof(task_rt));
    int priority = new_node->data.priority;
    new_node->priority = priority;
    
    // Insert at the correct position based on priority
    if (!pq->head || priority > pq->head->priority) {
        new_node->next = pq->head;
        pq->head = new_node;
    } else {
        pq_node_t *cur = pq->head;
        while (cur->next && cur->next->priority >= priority) {
            cur = cur->next;
        }
        new_node->next = cur->next;
        cur->next = new_node;
    }

    // release(&pq->lock);
    return 0;
}

// Remove and return the highest-priority item
int
pq_pop(priority_queue_t *pq, task_rt* dst) {
    // acquire(&pq->lock);
    
    /* queue is empty */
    if (!pq->head) {
    //    release(&pq->lock);
        return -1; 
    }

    pq_node_t *node = pq->head;
    pq->head = node->next;

    // task_rt data = node->data;
    memmove(dst, &node->data, sizeof(task_rt));
    pq_free_node(pq, node);

    // release(&pq->lock);
    return 0;
}
