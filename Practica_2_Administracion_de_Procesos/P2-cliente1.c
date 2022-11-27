#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

int main(int argc, char *argv[]){
    printf("Mi id process es %d\n", getpid());    
    printf("Enviare una señal al proceso agregado en argv[1] que es '%s'\n", argv[1]);
    if (kill(atoi(argv[1]), SIGIO) == -1){
        perror("Fallo al enviar la señal");
        exit(1);
    }
    return 0;
}