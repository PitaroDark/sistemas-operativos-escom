#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>

#define SHMKEY 75

int main()
{
    puts("Venta de boletos");
    int shmid;
    char *addr, *pint;
    /* Crear la región de memoria y obtener la dirección */
    shmid = shmget(SHMKEY, 1, 0777 | IPC_CREAT);
    /* Enlazar región al espacio de direccionamiento del proceso */
    addr = shmat(shmid, 0, 0);
    pint = (char *)addr; /* Reservar addr */
    /* Código de la sincronización */
    sleep(10); /* Para dar tiempo a que se ejecute el segundo proceso */
    (*pint)++;
    /* Separar la región del espacio de direccionamiento del proceso */
    shmdt(addr);
    /* Eliminar la región de memoria compartida */
    shmctl(shmid, IPC_RMID, 0); /* Esta operación también la puede realizar otro proceso */
    return 0;
}