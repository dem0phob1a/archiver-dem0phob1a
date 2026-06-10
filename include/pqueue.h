#ifndef PQUEUE_H
#define PQUEUE_H

#include <stddef.h>

/* Min-heap priority queue.
 * Lower priority value = higher priority (pops smallest first). */

typedef struct {
    void *data;
    int   priority;
} PQNode;

typedef struct {
    PQNode *heap;
    size_t  size;
    size_t  capacity;
} PQueue;

/* Create queue with given initial capacity. Returns NULL on failure. */
PQueue *pqueue_create(size_t initial_capacity);

/* Free queue structure (does NOT free data pointers). */
void    pqueue_destroy(PQueue *pq);

/* Insert element. Returns 0 on success, -1 on failure. */
int     pqueue_insert(PQueue *pq, void *data, int priority);

/* Remove and return minimum-priority element.
 * Writes priority to *out_priority if not NULL.
 * Returns NULL if queue is empty. */
void   *pqueue_pop(PQueue *pq, int *out_priority);

/* Peek at minimum element without removing it. Returns NULL if empty. */
void   *pqueue_peek(const PQueue *pq, int *out_priority);

/* Current number of elements. */
size_t  pqueue_size(const PQueue *pq);

#endif /* PQUEUE_H */
