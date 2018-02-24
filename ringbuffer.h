#ifndef RING_BUFFER_H
#define RING_BUFFER_H

#include <stddef.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>


#define POWEROF2(x)  ((((x) -1) & (x)) == 0)

#define CACHE_LINE_SIZE     64
#define CACHE_LINE_MASK     (CACHE_LINE_SIZE -1)

#define CACHE_LINE_ROUNDUP(size) \
    (CACHE_LINE_SIZE * ((size + CACHE_LINE_SIZE -1)/CACHE_LINE_SIZE))

typedef struct ring {
    unsigned int    size;
    unsigned int    mask;
    volatile unsigned int head;
    volatile unsigned int tail;
    void *volatile ring[0];
} ring_t;


static inline ring_t *ring_create(unsigned int size)
{
    ring_t *r;
    size_t ring_size;

    if(!POWEROF2(size))
        return NULL;

    ring_size = size * sizeof(void *) + sizeof(ring_t);

    r = malloc(ring_size);
    if(!r)
        return NULL;
    memset(r, 0, ring_size);
    r->size = size;
    r->mask = size - 1;

    return r;
}

static inline void ring_destroy(ring_t *r)
{
    free(r);
}

static inline unsigned int ring_length(ring_t *r)
{
    if(r->tail <= r->head)
        return (r->head - r->tail);
    else
        return (r->size - r->tail + r->head);
}

static inline bool ring_isempty(ring_t *r)
{
    return (r->head == r->tail);
}

static inline bool ring_isfull(ring_t *r)
{
    return ((r->head + 1) & r->mask) == r->tail;
}

static inline void *ring_peek(ring_t *r)
{
    if(r->head == r->tail)
        return NULL;

    return r->ring[r->tail];
}

static inline bool ring_enqueue(ring_t *r, void *data)
{
    if(((r->head +1 ) & r->mask) == r->tail)
        return false;

    r->ring[r->head] = data;
    r->head = (r->head + 1) & r->mask;
    //r->head = (__sync_fetch_and_add(&r->head, 1)) & r->mask;

    return true;
}

static inline void *ring_dequeue(ring_t *r)
{
    if(r->head == r->tail)
        return NULL;

    r->tail = (r->tail +1) & r->mask;
    //r->tail = (__sync_fetch_and_add(&r->tail, 1)) & r->mask;
    return r->ring[(r->tail -1) & r->mask];
}

#endif
