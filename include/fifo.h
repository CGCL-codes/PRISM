// fifo.h - FIFO + WRAM buffer pool design, supporting asynchronous Loader/Worker mode
#ifndef FIFO_H
#define FIFO_H

#include <stdint.h>
#include <stdbool.h>
#include <attributes.h> // for __mram_ptr
#include "common.h"     // basic types like node_t
#include <mutex.h>
#include <sem.h>
#include <stddef.h>


#define WRAM_FIFO_CAPACITY 64 // Maximum number of jobs (limited by WRAM size)
#define WRAM_MAX_ROOT_BUF_SLOT 16
#define WRAM_MAX_SECOND_BUF_SLOT 32
#define WRAM_BUF_SIZE 32


// ---------------- Root WRAM buffer metadata ----------------
typedef struct
{
    node_t root_id;
    node_t *ptr; // Pointer to actual WRAM buffer
    uint32_t size;
    uint32_t ref_count;
    bool in_use;
} a_buf_entry_t;

// ---------------- Secondary WRAM buffer metadata ----------------
typedef struct
{
    node_t second_id;
    node_t *ptr;
    uint32_t size;
    bool in_use;
} b_buf_entry_t;

// ---------------- Job structure ----------------
typedef struct {
    node_t root_id;
    uint8_t a_index;
    uint8_t b_index;
    uint8_t a_offset;
    uint8_t b_offset;
    uint32_t a_size;
    uint32_t b_size;
    node_t threshold;
} job_t;

// ---------------- FIFO definition ----------------
typedef struct
{
    job_t buffer[WRAM_FIFO_CAPACITY];
    volatile uint32_t head;
    volatile uint32_t tail;
    volatile uint8_t lock;
} fifo_t;

__dma_aligned a_buf_entry_t a_buf_table[WRAM_MAX_ROOT_BUF_SLOT];
__dma_aligned b_buf_entry_t b_buf_table[WRAM_MAX_SECOND_BUF_SLOT];
__dma_aligned node_t a_buf_pool[WRAM_MAX_ROOT_BUF_SLOT][WRAM_BUF_SIZE];
__dma_aligned node_t b_buf_pool[WRAM_MAX_SECOND_BUF_SLOT][WRAM_BUF_SIZE];
__dma_aligned fifo_t global_fifo;
__dma_aligned bool loader_done_flag;

MUTEX_INIT(my_fifo_lock);
MUTEX_INIT(a_buf_lock);
MUTEX_INIT(b_buf_lock);

// SEMAPHORE_INIT(fifo_empty_slots, WRAM_FIFO_CAPACITY);
// SEMAPHORE_INIT(fifo_full_slots, 0);
SEMAPHORE_INIT(a_buf_sem, WRAM_MAX_ROOT_BUF_SLOT);
SEMAPHORE_INIT(b_buf_sem, WRAM_MAX_SECOND_BUF_SLOT);

static inline void fifo_init(fifo_t *fifo)
{
    fifo->head = 0;
    fifo->tail = 0;
    fifo->lock = 0;
}

static bool fifo_is_empty(fifo_t *fifo)
{
    return fifo->head == fifo->tail;
}

static bool fifo_is_full(fifo_t *fifo)
{
    return ((fifo->tail + 1) % WRAM_FIFO_CAPACITY) == fifo->head;
}

static void fifo_lock_acquire()
{
    mutex_lock(my_fifo_lock);
}

static void fifo_lock_release()
{
    mutex_unlock(my_fifo_lock);
}

bool fifo_enqueue(fifo_t *fifo, job_t job)
{
    fifo_lock_acquire();
    if (fifo_is_full(fifo))
    {
        fifo_lock_release();
        return false;
    }
    fifo->buffer[fifo->tail] = job;
    fifo->tail = (fifo->tail + 1) % WRAM_FIFO_CAPACITY;
    fifo_lock_release();
    return true;
}


bool fifo_dequeue(fifo_t *fifo, job_t *job)
{
    fifo_lock_acquire();
    if (fifo_is_empty(fifo))
    {
        fifo_lock_release();
        return false;
    }
    *job = fifo->buffer[fifo->head];
    fifo->head = (fifo->head + 1) % WRAM_FIFO_CAPACITY;
    fifo_lock_release();
    return true;
}

static inline int allocate_a_buf(node_t size)
{
    sem_take(&a_buf_sem);
    mutex_lock(a_buf_lock);
    for (int i = 0; i < WRAM_MAX_ROOT_BUF_SLOT; i++) {
        if (!a_buf_table[i].in_use) {
            a_buf_table[i].in_use = true;
            a_buf_table[i].size = size;
            mutex_unlock(a_buf_lock);
            return i;
        }
    }
    mutex_unlock(a_buf_lock);
    return -1; // Should not happen in theory
}

static inline void release_a_buf(int index)
{
    mutex_lock(a_buf_lock); // <<<< lock
    a_buf_table[index].in_use = false;
    mutex_unlock(a_buf_lock); // <<<< unlock
    sem_give(&a_buf_sem);
}

static inline int allocate_b_buf(node_t size)
{
    //sem_take(&b_buf_sem);
    mutex_lock(b_buf_lock);
    for (int i = 0; i < WRAM_MAX_SECOND_BUF_SLOT; i++) {
        if (!b_buf_table[i].in_use) {
            b_buf_table[i].in_use = true;
            b_buf_table[i].size = size;
            mutex_unlock(b_buf_lock);
            return i;
        }
    }
    mutex_unlock(b_buf_lock);
    return -1; // Should not happen in theory
}

static inline void release_b_buf(int index)
{
    mutex_lock(b_buf_lock);
    b_buf_table[index].in_use = false;
    mutex_unlock(b_buf_lock);
    //sem_give(&b_buf_sem);
}

#endif // FIFO_H
