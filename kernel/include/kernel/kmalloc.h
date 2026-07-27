#ifndef _KERNEL_KMALLOC_H
#define _KERNEL_KMALLOC_H
 
#include <stddef.h>

 
void* kmalloc(size_t size);
void kfree(void* ptr);
void* krealloc(void* ptr, size_t size);
 
#endif // _KERNEL_KMALLOC_H
 