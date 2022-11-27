#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <signal.h>

void señalIO(int );

int main(int argc, char *argv[]){
    printf("Yo soy el servidor\n");
    signal(SIGIO, &señalIO);
    while (1)
    {
        printf("Estoy en ejecucion\n");
        printf("Te puedes conectar a mi con mi id '%d'\n", getpid());
        sleep(2);
    }
    return 0;
}

void señalIO(int signal){
    char enter;
    printf("Te acabas de comunicar con el servidor\n");
    printf("Esperando muerte...\n");
    scanf("%c", &enter);
    exit(0);
}