#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

typedef struct {
	size_t size;
	uint32_t magic;
	int is_free;
} malloc_metadata_t;

#define MALLOC_MAGIC 0xDEADBEEF
#define METADATA_SIZE sizeof(malloc_metadata_t)

extern char _heap_start;
extern char _heap_end;

static malloc_metadata_t* heap_base = NULL;
static malloc_metadata_t* heap_ptr = NULL;

static void malloc_init(void)
{
	if (heap_base == NULL) {
		heap_base = (malloc_metadata_t*) &_heap_start;
		heap_ptr = heap_base;
	}
}
static malloc_metadata_t* next_block(malloc_metadata_t* block)
{
    char* next = (char*)(block + 1) + block->size;
    if (next >= &_heap_end) {
        return NULL;
    }
    return (malloc_metadata_t*)next;
}

static malloc_metadata_t* prev_block(malloc_metadata_t* block)
{
    malloc_metadata_t* cur = heap_base;
    malloc_metadata_t* prev = NULL;

    while ((char*)cur < &_heap_end && cur != block) {
        if (cur->magic != MALLOC_MAGIC) {
            return NULL;
        }

        prev = cur;
        cur = (malloc_metadata_t*)((char*)(cur + 1) + cur->size);
    }

    return prev;
}

static malloc_metadata_t* find_free_block(size_t size)
{
	malloc_metadata_t* block = heap_base;
	
	while ((char*)block < &_heap_end) {
		if (block->magic != MALLOC_MAGIC) {
			return NULL;
		}
		
		if (block->is_free && block->size >= size) {
			return block;
		}
		
		block = (malloc_metadata_t*)((char*)(block + 1) + block->size);
	}
	
	return NULL;
}
static void coalesce(malloc_metadata_t* block)
{
    /* Forward coalescing */
    malloc_metadata_t* next = next_block(block);
    if (next && next->magic == MALLOC_MAGIC && next->is_free) {
        block->size += METADATA_SIZE + next->size;

        if (next == heap_ptr) {
            heap_ptr = block;
        }
    }

    /* Backward coalescing */
    malloc_metadata_t* prev = prev_block(block);
    if (prev && prev->magic == MALLOC_MAGIC && prev->is_free) {
        prev->size += METADATA_SIZE + block->size;

        if (block == heap_ptr) {
            heap_ptr = prev;
        }
    }
}

static malloc_metadata_t* expand_heap(size_t size)
{
	char* next_block = (char*)(heap_ptr + 1) + heap_ptr->size;
	if (next_block + METADATA_SIZE + size > &_heap_end) {
		return NULL;
	}
	
	malloc_metadata_t* new_block = (malloc_metadata_t*)(next_block);
	new_block->size = size;
	new_block->magic = MALLOC_MAGIC;
	new_block->is_free = 0;
	
	heap_ptr = new_block;
	return new_block;
}

void* malloc(size_t size)
{
	if (size == 0) {
		return NULL;
	}
	
	malloc_init();
	
	malloc_metadata_t* block = find_free_block(size);
	
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

void free(void* ptr)
{
    if (ptr == NULL) {
        return;
    }

    malloc_metadata_t* block = (malloc_metadata_t*)ptr - 1;

    if (block->magic != MALLOC_MAGIC) {
        return;
    }

    block->is_free = 1;
    coalesce(block);
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
	
	malloc_init();
	
	malloc_metadata_t* block = (malloc_metadata_t*)ptr - 1;
	
	if (block->magic != MALLOC_MAGIC) {
		return NULL;
	}
	
	if (block->size >= size) {
		return ptr;
	}
	
	void* new_ptr = malloc(size);
	if (new_ptr == NULL) {
		return NULL;
	}
	
	memcpy(new_ptr, ptr, block->size);
	
	free(ptr);
	
	return new_ptr;
}
