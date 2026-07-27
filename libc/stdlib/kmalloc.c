#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <kernel/kmalloc.h>

typedef struct {
	size_t size;
	uint32_t magic;
	int is_free;
} kmalloc_metadata_t;

#define KMALLOC_MAGIC 0xDEADBEEF
#define KMALLOC_METADATA_SIZE sizeof(kmalloc_metadata_t)

extern char _heap_start;
extern char _heap_end;

static kmalloc_metadata_t* heap_base = NULL;
static kmalloc_metadata_t* heap_ptr = NULL;

static void kmalloc_init(void)
{
	if (heap_base == NULL) {
		heap_base = (kmalloc_metadata_t*) &_heap_start;
		heap_ptr = heap_base;
	}
}
static kmalloc_metadata_t* next_block(kmalloc_metadata_t* block)
{
    char* next = (char*)(block + 1) + block->size;
    if (next >= &_heap_end) {
        return NULL;
    }
    return (kmalloc_metadata_t*)next;
}

static kmalloc_metadata_t* prev_block(kmalloc_metadata_t* block)
{
    kmalloc_metadata_t* cur = heap_base;
    kmalloc_metadata_t* prev = NULL;

    while ((char*)cur < &_heap_end && cur != block) {
        if (cur->magic != KMALLOC_MAGIC) {
            return NULL;
        }

        prev = cur;
        cur = (kmalloc_metadata_t*)((char*)(cur + 1) + cur->size);
    }

    return prev;
}

static kmalloc_metadata_t* find_free_block(size_t size)
{
	kmalloc_metadata_t* block = heap_base;
	
	while ((char*)block < &_heap_end) {
		if (block->magic != KMALLOC_MAGIC) {
			return NULL;
		}
		
		if (block->is_free && block->size >= size) {
			return block;
		}
		
		block = (kmalloc_metadata_t*)((char*)(block + 1) + block->size);
	}
	
	return NULL;
}
static void coalesce(kmalloc_metadata_t* block)
{
    /* Forward coalescing */
    kmalloc_metadata_t* next = next_block(block);
    if (next && next->magic == KMALLOC_MAGIC && next->is_free) {
        block->size += KMALLOC_METADATA_SIZE + next->size;

        if (next == heap_ptr) {
            heap_ptr = block;
        }
    }

    /* Backward coalescing */
    kmalloc_metadata_t* prev = prev_block(block);
    if (prev && prev->magic == KMALLOC_MAGIC && prev->is_free) {
        prev->size += KMALLOC_METADATA_SIZE + block->size;

        if (block == heap_ptr) {
            heap_ptr = prev;
        }
    }
}

static kmalloc_metadata_t* expand_heap(size_t size)
{
	char* next_block = (char*)(heap_ptr + 1) + heap_ptr->size;
	if (next_block + KMALLOC_METADATA_SIZE + size > &_heap_end) {
		return NULL;
	}
	
	kmalloc_metadata_t* new_block = (kmalloc_metadata_t*)(next_block);
	new_block->size = size;
	new_block->magic = KMALLOC_MAGIC;
	new_block->is_free = 0;
	
	heap_ptr = new_block;
	return new_block;
}

void* kmalloc(size_t size)
{
	if (size == 0) {
		return NULL;
	}
	
	kmalloc_init();
	
	kmalloc_metadata_t* block = find_free_block(size);
	
	if (block == NULL) {
		block = expand_heap(size);
		if (block == NULL) {
			return NULL;
		}
	} else {
		block->is_free = 0;
	}
	
	return (void*)(block + 1);
}

void kfree(void* ptr)
{
    if (ptr == NULL) {
        return;
    }

    kmalloc_metadata_t* block = (kmalloc_metadata_t*)ptr - 1;

    if (block->magic != KMALLOC_MAGIC) {
        return;
    }

    block->is_free = 1;
    coalesce(block);
}


void* krealloc(void* ptr, size_t size)
{
	if (size == 0) {
		kfree(ptr);
		return NULL;
	}
	
	if (ptr == NULL) {
		return kmalloc(size);
	}
	
	kmalloc_init();
	
	kmalloc_metadata_t* block = (kmalloc_metadata_t*)ptr - 1;
	
	if (block->magic != KMALLOC_MAGIC) {
		return NULL;
	}
	
	if (block->size >= size) {
		return ptr;
	}
	
	void* new_ptr = kmalloc(size);
	if (new_ptr == NULL) {
		return NULL;
	}
	
	memcpy(new_ptr, ptr, block->size);
	
	kfree(ptr);
	
	return new_ptr;
}