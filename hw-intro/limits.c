#include <stdio.h>
#include <sys/resource.h>

int main() {
    struct rlimit lim;
    if (getrlimit(RLIMIT_CPU, &lim) == 0){
        printf("process limit: %ld\n", lim.rlim_cur); //RLIMIT_CPU
    }
    else{
        perror("An Error occured with getting the stack limit");
    }

    if (getrlimit(RLIMIT_STACK, &lim) == 0){
        printf("stack size: %ld\n", lim.rlim_cur); //RLIMIT_STACK
    }
    else{
        perror("An Error occured with getting the stack limit");
    }
    if (getrlimit(RLIMIT_NOFILE, &lim) == 0){
        printf("max file descriptors: %ld\n", lim.rlim_cur); //RLIMIT_NOFILE
    }
    else{
        perror("An Error occured with getting the stack limit");
    }
    return 0;
}
