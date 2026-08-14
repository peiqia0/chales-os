#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <syscalls.h>

/* User-space heap allocator. Unlike kmalloc() (kernel/include/kernel/kmalloc.h),
 * this doesn't touch kernel memory directly — it grows the process's heap by
 * asking the kernel via the sbrk() syscall, which only ever hands back memory
 * from the dedicated user-heap region (_user_heap_start.._user_heap_end) that
 * paging_initialize() marks PTE_USER. This file is linked into the .user
 * section (see linker.ld) so it runs at ring 3 alongside main.c/printf.c.
 */

typedef struct block_header {
    size_t size;
    uint32_t magic;
    int is_free;
    struct block_header* next;
} block_header_t;

#define MALLOC_MAGIC 0xCAFEF00D
#define HEADER_SIZE sizeof(block_header_t)

static block_header_t* heap_head = NULL;
static block_header_t* heap_tail = NULL;

/* Ask the kernel for `size` more bytes of heap and carve a new block out of
 * it. Returns NULL if the user heap region is exhausted. */
static block_header_t* request_block(size_t size)
{
    void* prev_brk = _syscall_sbrk((int)(HEADER_SIZE + size));
    if (prev_brk == (void*)-1) {
        return NULL;
    }

    block_header_t* block = (block_header_t*)prev_brk;
    block->size = size;
    block->magic = MALLOC_MAGIC;
    block->is_free = 0;
    block->next = NULL;

    if (heap_tail) {
        heap_tail->next = block;
    } else {
        heap_head = block;
    }
    heap_tail = block;

    return block;
}

static block_header_t* find_free_block(size_t size)
{
    for (block_header_t* block = heap_head; block; block = block->next) {
        if (block->is_free && block->size >= size) {
            return block;
        }
    }
    return NULL;
}

void* malloc(size_t size)
{
    if (size == 0) {
        return NULL;
    }

    block_header_t* block = find_free_block(size);
    if (block) {
        block->is_free = 0;
    } else {
        block = request_block(size);
        if (!block) {
            return NULL;
        }
    }

    return (void*)(block + 1);
}

void free(void* ptr)
{
    if (ptr == NULL) {
        return;
    }

    block_header_t* block = (block_header_t*)ptr - 1;
    if (block->magic != MALLOC_MAGIC) {
        return;
    }

    block->is_free = 1;

    /* Coalesce adjacent free blocks to fight fragmentation, since we never
     * give memory back to the kernel (no shrinking sbrk yet). */
    for (block_header_t* b = heap_head; b && b->next; b = b->next) {
        if (b->is_free && b->next->is_free) {
            b->size += HEADER_SIZE + b->next->size;
            if (b->next == heap_tail) {
                heap_tail = b;
            }
            b->next = b->next->next;
        }
    }
}

void* realloc(void* ptr, size_t size)
{
    if (size == 0) {
        free(ptr);
        return NULL;
    }

    if (ptr == NULL) {
        return malloc(size);
    }

    block_header_t* block = (block_header_t*)ptr - 1;
    if (block->magic != MALLOC_MAGIC) {
        return NULL;
    }

    if (block->size >= size) {
        return ptr;
    }

    void* new_ptr = malloc(size);
    if (!new_ptr) {
        return NULL;
    }

    memcpy(new_ptr, ptr, block->size);
    free(ptr);

    return new_ptr;
}
