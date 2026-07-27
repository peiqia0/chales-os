#include <stdio.h>
#include <stdlib.h>
#include <syscalls.h>

void user_main() {

    // Initially allocate memory for 5 integers
    int *ptr = (int *)malloc(5 * sizeof(int));
    
    // Check if allocation was successful
    if (ptr == NULL) {
        printf("Memory Allocation Failed\n");
        _syscall_exit(0);

    }
    
    // Now, we need to store 8 elements so
    // Reallocate to store 8 integers
    ptr = (int *)realloc(ptr, 8 * sizeof(int)); 
    
    // Check if reallocation was successful
    if (ptr == NULL) {
        printf("Memory Reallocation Failed\n");
        _syscall_exit(0);
    }
    
    // Assume we only use 5 elements now
    for (int i = 0; i < 5; i++) {
        ptr[i] = (i + 1) * 10;
    }
    
    // Shrink the array back to 5 elements
    ptr = (int *)realloc(ptr, 5 * sizeof(int));
    
    // Check if shrinking was successful
    if (ptr == NULL) {
        printf("Memory Reallocation Failed\n");
        _syscall_exit(0);
    }
    
    for (int i = 0; i < 5; i++)
        printf("%d ", ptr[i]);
    
    // Finally, free the memory when done
    free(ptr);

    _syscall_exit(0);
    // return 0;
}