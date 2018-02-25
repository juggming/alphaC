#ifndef _LINUX_KFIFO_H
#define _LINUX_KFIFO_H

/*
 * Note about locking : There is no locking required until only * one reader
 * and one writer is using the fifo and no kfifo_reset() will be * called
 *  kfifo_reset_out() can be safely used, until it will be only called
 * in the reader thread.
 *  For multiple writer and one reader there is only a need to lock the writer.
 * And vice versa for only one writer and multiple reader there is only a need
 * to lock the reader.
 */

#include "linux/bitopts.h"
#include <stdlib.h>
#include <errno.h>
#include <string.h>


#define smp_rmb()       asm volatile("lfence":::"memory")
#define smp_mb()        asm volatile("mfence":::"memory")
#define smp_wmb()       asm volatile("sfence":::"memory")

#define min(a, b) ((a) < (b) ? (a) : (b))

typedef struct kfifo {
	unsigned int	in;
	unsigned int	out;
	unsigned int	mask;
	unsigned int	esize;  //elements size
	void		*data;
} kfifo_t;

int kfifo_init(struct kfifo *fifo, void *buffer,
        unsigned int size, size_t esize)
{
    size /= esize;

    if(!is_power_of_2(size))
        size = rounddown_pow_of_two(size);

    fifo->in = 0;
    fifo->out = 0;
    fifo->esize = esize;
    fifo->data = buffer;

    if(size < 2) {
        fifo->mask = 0;
        return -EINVAL;
    }
    fifo->mask = size - 1;

    return 0;
}

int kfifo_alloc(struct kfifo *fifo, unsigned int size, size_t esize)
{
    /*
     * round down to the next power of 2, since our 'let indices wrap'
     * technique works only in this case
     */
    if(!is_power_of_2(size))
        size = rounddown_pow_of_two(size);

    fifo->in = 0;
    fifo->out = 0;
    fifo->esize = esize;

    if(size < 2) {
        fifo->data = NULL;
        fifo->mask = 0;
        return -EINVAL;
    }

    fifo->data = malloc(size * esize);
    if(!fifo->data) {
        fifo->mask = 0;
        return -ENOMEM;
    }
    fifo->mask = size - 1;

    return 0;
}

void kfifo_free(struct kfifo *fifo)
{
    free(fifo->data);
    fifo->in = fifo->out = fifo->esize = 0;
    fifo->data = NULL;
    fifo->mask = 0;
}

static inline unsigned int kfifo_unused(struct kfifo *fifo)
{
    return (fifo->mask + 1) - (fifo->in  - fifo->out);
}

static void kfifo_copy_in(struct kfifo *fifo, const void *src,
        unsigned int len, unsigned int off)
{
    unsigned int size = fifo->mask + 1;
    unsigned int esize = fifo->esize;
    unsigned int l;

    off &= fifo->mask;
    if(esize != 1) {
        off *= esize;
        size *= esize;
        len *= esize;
    }
    l = min(len, size - off);

    memcpy(fifo->data + off, src, l);
    memcpy(fifo->data, src + l, len - l);

    /*
     * make sure that the data in the fifo is up to data before
     * incrementing the fifo->in index counter
     */
    smp_wmb();
}

static void kfifo_copy_out(struct kfifo *fifo, void *dst,
        unsigned int len, unsigned int off)
{
    unsigned int size = fifo->mask + 1;
    unsigned int esize = fifo->esize;
    unsigned int l;

    off &= fifo->mask;
    if(esize != 1) {
        off *= esize;
        size *= esize;
        len *= esize;
    }

    l = min(len, size - off);

    memcpy(dst, fifo->data + off, l);
    memcpy(dst + l, fifo->data, len - l);

    /*
     * make sure that the data in the fifo is up to data before
     * incrementing the fifo->in index counter
     */
    smp_wmb();
}

unsigned int kfifo_in(struct kfifo *fifo, const void *buf, unsigned int len)
{
    unsigned int l;

    l = kfifo_unused(fifo);
    if(len > l)
        len = l;

    kfifo_copy_in(fifo, buf, len, fifo->in);
    fifo->in += len;

    return len;
}

unsigned int kfifo_out_peek(struct kfifo *fifo, void *buf, unsigned int len)
{
    unsigned int l;

    l = fifo->in - fifo->out;
    if(len > l)
        len = l;

    kfifo_copy_out(fifo, buf, len, fifo->out);
    return len;
}

unsigned int kfifo_out(struct kfifo *fifo, void *buf, unsigned int len)
{
    len = kfifo_out_peek(fifo, buf, len);
    fifo->out += len;
    return len;
}

#endif
