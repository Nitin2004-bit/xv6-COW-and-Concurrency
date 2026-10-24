#include "kernel/types.h"
#include "kernel/memlayout.h"
#include "user/user.h"

void read_only_test()
{
    int sz = 4096; // allocate 1 page
    char *p = sbrk(sz); 
    if (p == (char*)-1) {
        printf("sbrk failed\n");
        exit(1);
    }
    p[0] = 'A';  // Parent writes to memory

    int pid = fork();
    if (pid < 0) {
        printf("fork failed\n");
        exit(1);
    }
    
    if (pid == 0) {
        // Child reads the memory, doesn't modify it
        if (p[0] != 'A') {
            printf("Child failed to read correctly\n");
            exit(1);
        }
        printf("Read-only test passed\n");
        exit(0);
    }

    wait(0);  // Parent waits for child
    printf("Read-only test passed\n");

    printf("%d\n",get_cow_fault_count);
    reset_cow_fault_count();
}

int main(){
    printf("%d\n",get_cow_fault_count);
    read_only_test();
    return 0;
}