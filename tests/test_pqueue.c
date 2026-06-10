#include "../include/pqueue.h"
#include <stdio.h>
#include <assert.h>

static int passed = 0, total = 0;
#define TEST(name)   do { total++; printf("  %-50s", name); } while(0)
#define PASS()       do { passed++; puts("PASS"); } while(0)
#define FAIL(msg)    do { puts("FAIL: " msg); } while(0)

static void test_create_destroy(void)
{
    TEST("create / destroy");
    PQueue *pq = pqueue_create(8);
    assert(pq);
    assert(pqueue_size(pq) == 0);
    pqueue_destroy(pq);
    PASS();
}

static void test_insert_pop_min_order(void)
{
    TEST("pop returns minimum priority first");
    PQueue *pq = pqueue_create(4);
    pqueue_insert(pq, (void*)5L, 5);
    pqueue_insert(pq, (void*)1L, 1);
    pqueue_insert(pq, (void*)3L, 3);
    pqueue_insert(pq, (void*)2L, 2);
    pqueue_insert(pq, (void*)4L, 4);

    int prev = -1, ok = 1;
    while (pqueue_size(pq) > 0) {
        int p; pqueue_pop(pq, &p);
        if (p < prev) { ok = 0; break; }
        prev = p;
    }
    pqueue_destroy(pq);
    if (ok) PASS(); else FAIL("wrong order");
}

static void test_peek_no_remove(void)
{
    TEST("peek does not remove element");
    PQueue *pq = pqueue_create(4);
    pqueue_insert(pq, NULL, 42);
    int p; pqueue_peek(pq, &p);
    int ok = (pqueue_size(pq) == 1 && p == 42);
    pqueue_destroy(pq);
    if (ok) PASS(); else FAIL("size changed or wrong priority");
}

static void test_autogrow(void)
{
    TEST("auto-grow beyond initial capacity");
    PQueue *pq = pqueue_create(2);
    for (int i = 200; i >= 1; i--)
        pqueue_insert(pq, NULL, i);
    int prev = -1, ok = 1;
    while (pqueue_size(pq)) {
        int p; pqueue_pop(pq, &p);
        if (p < prev) { ok = 0; break; }
        prev = p;
    }
    pqueue_destroy(pq);
    if (ok) PASS(); else FAIL("order wrong after grow");
}

static void test_pop_empty(void)
{
    TEST("pop on empty queue returns NULL");
    PQueue *pq = pqueue_create(4);
    void *r = pqueue_pop(pq, NULL);
    pqueue_destroy(pq);
    if (!r) PASS(); else FAIL("expected NULL");
}

static void test_same_priority(void)
{
    TEST("multiple elements with same priority");
    PQueue *pq = pqueue_create(8);
    for (int i = 0; i < 5; i++) pqueue_insert(pq, NULL, 7);
    int ok = 1;
    while (pqueue_size(pq)) {
        int p; pqueue_pop(pq, &p);
        if (p != 7) { ok = 0; break; }
    }
    pqueue_destroy(pq);
    if (ok) PASS(); else FAIL("unexpected priority");
}

int main(void)
{
    puts("=== Priority Queue Tests ===");
    test_create_destroy();
    test_insert_pop_min_order();
    test_peek_no_remove();
    test_autogrow();
    test_pop_empty();
    test_same_priority();
    printf("\n%d / %d passed.\n", passed, total);
    return passed == total ? 0 : 1;
}
