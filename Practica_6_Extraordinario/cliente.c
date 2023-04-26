#include <stdio.h>
#include <stdio_ext.h>
#include <stdbool.h>
#include <string.h>
#include <curses.h>
#include "unidad2.h"

#define MAX_SHM_STRING 200

void getPasswordAsterisk(char pass[]);

int main(){

    bool login = true;
    int contador = 0, llave, archivo;
    int semServidor, semClaves, semCliente, semHilo; //SEMAFOROS
    int *claves_hilo = NULL, *chat = NULL; //MEMORIAS COMPARTIDAS
    char *chatStr = NULL;
    char numero_archivo[4], archivo_shm[8], archivo_sem_cliente[16];
    char archivo_sem_hilo[13], archivo_shm_str[15];

    /* ------------------ Memorias compartidas del servidor ------------------- */
    /* Conexión a la memoria compartida para la asignación de claves de comunicación con el hilo */
    claves_hilo = (int *)shm(sizeof(int)*2, "shm_claves", 'v');

    /* ------------------------ Semáforos del servidor ------------------------ */
    semServidor = sem("sem_servidor", 'w'); /* Semáforo del servidor */
    semCliente = sem("sem_cliente", 'x'); /* Semáforo del cliente */
    semClaves = sem("sem_claves", 'y'); /* Semáforo de claves */

    /* ----------------- Obtención de claves de comunicación ------------------ */
    down(semServidor, 0);/* El servidor se ocupa */
    up(semClaves, 0); // Desocupamos el semaforo claves
    down(semCliente, 0); //Ocupamos el semaforo cliente

    /* Obtención de las claves del servidor */
    llave = claves_hilo[0];
    archivo = claves_hilo[1];

    up(semClaves, 0);
    down(semCliente, 0);
    up(semServidor, 0);/* Se libera el servidor */

    /* ------------------- Obtención de nombres de archivos ------------------- */   
    sprintf(numero_archivo, "%d", archivo);/* Conversión del número de archivo asignado a cadena */

    /* Archivo: shm_xxx */
    strcpy(archivo_shm, "shm_");
    strcat(archivo_shm, numero_archivo);

    /* Archivo shm_xxx_str */
    strcpy(archivo_shm_str, archivo_shm);
    strcat(archivo_shm_str, "_str");

    /* Archivo: sem_cliente_xxx */
    strcpy(archivo_sem_cliente, "sem_cliente_");
    strcat(archivo_sem_cliente, numero_archivo);

    /* Archivo: sem_hilo_xxx */
    strcpy(archivo_sem_hilo, "sem_hilo_");
    strcat(archivo_sem_hilo, numero_archivo);

    /* -------------------- Memorias compartidas del hilo --------------------- */
    /* Chat's */
    chatStr = (char *)shm(sizeof(char)*MAX_SHM_STRING, archivo_shm_str, llave + 200);
    chat = (int *)shm(sizeof(int)*2, archivo_shm, llave);

    /* -------------------------- Semáforos del hilo -------------------------- */
    semHilo = sem(archivo_sem_hilo, llave+400);/* Semáforo del hilo */
    semCliente = sem(archivo_sem_cliente, llave+500);/* Semáforo del cliente */

    /* -------------------------- SISTEMA DE CONTROL -------------------------- */
    initscr(); //INICIAMOS DISPLAY ESPECIAL
    while(login){
        char name[20], user[20], pass[20];
        bool otraCompra = true;
        clear();
        printw("Inicio de sesion\n");
        printw("Usuario -> ");  scanw("%s", user);
        refresh();
        printw("Password -> "); getPasswordAsterisk(pass);
        //Enviamos los datos al servidor
        strcpy(chatStr, user); strcat(chatStr, ","); strcat(chatStr, pass);
        clear(); refresh();

        up(semHilo, 0);  //Desocupamos al hilo servidor
        down(semCliente, 0);//Esperamos al hilo servidor

        if(!chat[1]){
            printw("No se encontro el usuario, debera registrarse\n");
            printw("Registrese, llene el formulario sin espacios, porfavor\n");
            printw("Nombre de Usuario-> ");
            scanw("%s", user);
            printw("Nombre -> ");
            scanw("%s", name);
            printw("Password -> ");
            scanw("%s", pass);
            refresh();
            strcpy(chatStr, user); strcat(chatStr, ",");
            strcat(chatStr, name); strcat(chatStr, ",");
            strcat(chatStr, pass);
            up(semHilo, 0);
        }
        clear();
        printw("Bienvenido %s\n", user);
        while (otraCompra){
            refresh();
            int opc;
            printw("Que desea hacer\n");
            printw("1 -> Ver catalogo\n");
            printw("2 -> Ver carrito\n"); 
            printw("3 -> Comprar un libro\n");
            printw("4 -> Salir de sesion\n");
            printw("5 -> Salir de la aplicacion\n");
            printw("Opcion -> ");
            scanw("%i", &opc);
            chat[0] = opc;
            refresh();
            //up(semHilo, 0);  //Desocupamos al hilo servidor
            //down(semCliente, 0);//Esperamos al hilo servidor
            switch (opc){
            case 1: //MOSTRAR CATALOGO
                up(semHilo, 0);  //Desocupamos al hilo servidor
                down(semCliente, 0);//Esperamos al hilo servidor
                printw("\n----------------------------------\n");
                printw("ID\tNombre\t   Cantidad\n");
                printw("----------------------------------\n");
                printw("%s", chatStr);
                printw("----------------------------------\n");
                printw("Presione enter para continuar...\n");
                refresh();
                getch();
                __fpurge(stdin);
                break;
            case 2: //MOSTRAR Y QUITAR ELEMENTOS CARRITO
                int opcDelete = 0;
                strcpy(chatStr, user);
                up(semHilo, 0);  //Desocupamos al hilo servidor
                down(semCliente, 0);//Esperamos al hilo servidor
                printw("\n----------------------------------\n");
                printw("ID\tNombre\tCantidad\n");
                printw("----------------------------------\n");
                printw("%s", chatStr);
                printw("----------------------------------\n\n");
                if(strlen(chatStr) > 1){
                    printw("¿Quieres quitar algun libro?\n");
                    printw("0 -> NO | 1 -> SI\n");
                    printw("Opcion -> ");
                    refresh();
                    scanw("%i", &opcDelete);
                    chat[1] = opcDelete;
                    up(semHilo, 0);  //Desocupamos al hilo servidor
                    down(semCliente, 0);//Esperamos al hilo servidor
                    if(chat[1] == true){
                        printw("\nIngresa el Id del libro que quieres quitar\n");
                        printw("Opc -> ");
                        refresh();
                        scanw("%s", chatStr);
                        up(semHilo, 0);  //Desocupamos al hilo servidor
                        down(semCliente, 0);//Esperamos al hilo servidor
                    }
                    else
                        up(semHilo, 0);  //Desocupamos al hilo servido
                    
                }
                printw("Presione enter para continuar...\n");
                refresh();
                getch();
                __fpurge(stdin);
                break;
            case 3: //AGREGAR O COMPRAR CARRITO
                int opc;
                strcpy(chatStr, user);
                up(semHilo, 0);  //Desocupamos al hilo servidor
                down(semCliente, 0);//Esperamos al hilo servidor
                printw("\nAgregar al carrito\n");
                printw("0 -> NO | 1 -> SI\n");
                printw("Opc -> ");
                refresh();
                scanw("%i", &opc);
                chat[1] = opc;
                up(semHilo, 0);
                if(chat[1] == true){
                    printw("\nIngrese el Id del libro -> ");
                    scanw("%s", chatStr);
                    up(semHilo, 0);
                    down(semCliente, 0);
                    printw("\n%s\n", chatStr);
                }
                printw("¿Deseas comprar todo lo de tu carrito?\n");
                printw("0 -> NO | 1 -> SI\n");
                printw("Opc -> ");
                refresh();
                scanw("%i", &opc);
                chat[1] = opc;
                up(semHilo, 0);
                if(chat[1] == true){ //compramos
                    down(semCliente, 0);
                    printw("%s",chatStr);
                    refresh();
                }
                printw("Presione enter para continuar...\n");
                getch();
                break;
            case 4: //LOG OUT
                up(semHilo, 0);  //Desocupamos al hilo servidor
                down(semCliente, 0);//Esperamos al hilo servidor
                //up(semHilo, 0);  //Desocupamos al hilo servidor
                otraCompra = false;
                break;
            case 5: //SALIR COMPLETAMENTE
                up(semHilo, 0);  //Desocupamos al hilo servidor
                down(semCliente, 0);//Esperamos al hilo servidor
                login = false;
                otraCompra = false;
                break;
            default:
                up(semHilo, 0);  //Desocupamos al hilo servidor
                down(semCliente, 0);//Esperamos al hilo servidor
                clear();
                printw("Selecione una opcion valida\n");
                refresh();
                sleep(1.5);
                break;
            }
            //WHILE2
            clear();
        }
        //WHILE 1
        refresh();
    }
    printw("Espero que haya disfrutado su visita\n");
    printw("Nos vemos...\n");
    refresh();
    sleep(4);
    endwin();
    return 0;
}

void getPasswordAsterisk(char pass[]){
    int contador = 0;
    while (pass[contador] = getch()){
        if(pass[contador] == 10){
            pass[contador] = '\0';
            break;
        }
        if(pass[contador] == 127){
            printw("\b\b  \b\b");
            if(contador > 0){
                contador--;
                printw("\b \b");
            }
        }
        else{
            contador++;
            printw("\b \b");
            printw("*");
        }
        refresh();
    }
}