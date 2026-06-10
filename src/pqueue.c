#include "pqueue.h"
#include <stdlib.h>

static void swap(PQNode *a, PQNode *b) { PQNode t = *a; *a = *b; *b = t; }

static void sift_up(PQueue *pq, size_t i)
{
    while (i > 0) {
        size_t p = (i - 1) / 2;
        if (pq->heap[p].priority > pq->heap[i].priority) {
            swap(&pq->heap[p], &pq->heap[i]);
            i = p;
        } else break;
    }
}

static void sift_down(PQueue *pq, size_t i)
{
    size_t n = pq->size;
    for (;;) {
        size_t l = 2*i+1, r = 2*i+2, s = i;
        if (l < n && pq->heap[l].priority < pq->heap[s].priority) s = l;
        if (r < n && pq->heap[r].priority < pq->heap[s].priority) s = r;
        if (s == i) break;
        swap(&pq->heap[i], &pq->heap[s]);
        i = s;
    }
}

PQueue *pqueue_create(size_t initial_capacity)
{
    if (initial_capacity == 0) initial_capacity = 16;
    PQueue *pq = (PQueue *)malloc(sizeof(PQueue));
    if (!pq) return NULL;
    pq->heap = (PQNode *)malloc(initial_capacity * sizeof(PQNode));
    if (!pq->heap) { free(pq); return NULL; }
    pq->size     = 0;
    pq->capacity = initial_capacity;
    return pq;
}

void pqueue_destroy(PQueue *pq)
{
    if (!pq) return;
    free(pq->heap);
    free(pq);
}

int pqueue_insert(PQueue *pq, void *data, int priority)
{
    if (!pq) return -1;
    if (pq->size == pq->capacity) {
        size_t nc = pq->capacity * 2;
        PQNode *nh = (PQNode *)realloc(pq->heap, nc * sizeof(PQNode));
        if (!nh) return -1;
        pq->heap = nh;
        pq->capacity = nc;
    }
    pq->heap[pq->size].data     = data;
    pq->heap[pq->size].priority = priority;
    sift_up(pq, pq->size);
    pq->size++;
    return 0;
}

void *pqueue_pop(PQueue *pq, int *out_priority)
{
    if (!pq || pq->size == 0) return NULL;
    void *result = pq->heap[0].data;
    if (out_priority) *out_priority = pq->heap[0].priority;
    pq->size--;
    if (pq->size > 0) {
        pq->heap[0] = pq->heap[pq->size];
        sift_down(pq, 0);
    }
    return result;
}

void *pqueue_peek(const PQueue *pq, int *out_priority)
{
    if (!pq || pq->size == 0) return NULL;
    if (out_priority) *out_priority = pq->heap[0].priority;
    return pq->heap[0].data;
}

size_t pqueue_size(const PQueue *pq) { return pq ? pq->size : 0; }
