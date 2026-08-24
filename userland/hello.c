#include <syscalls.h>
#include <stdio.h>

int main(int argc, char *argv[])
{
    printf("Hello from an ELF-loaded user program!\n");
    printf("%s\n", argv[1]);
    return 0;
}
