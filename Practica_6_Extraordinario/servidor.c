#include <stdio.h>
#include <stdio_ext.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <pthread.h>
#include <curses.h>
#include "unidad2.h"

#define MAX_CLIENTES 20
#define MAX_SHM_STRING 200

int lenghFile(char nameFc[]);
int getInventory(char id[]);
void showCarrito(char nombre[], char *comunicator);
void getBook(char id[], char data[]);
void showInventory(char *comunicator);
void serveCustomer(void *);
void removerCaracteres(char *cadena, char *caracteres);
bool reduceInventory(char id[]);
bool addCarrito(char user[], char *comunicator);
bool delCarrito(char user[], char *book);
bool buyCarrito(char user[], char *comunicator);
bool validUser(char *credentials);
bool saveUser(char *credentials);

int main(){
    /* ------------------------------ VARIABLES --------------------------------*/ 
    int contador = 0;
    int semServidor, semCliente, semClaves; //SEMAFOROS
    int *parametrosCliente = NULL; //MEMORIAS COMPARTIDAS
    int llaves_conexion[20][2], llave, archivo; //LLAVES
    char numero_archivo[4], archivo_shm[8], archivo_sem_cliente[16], archivo_sem_hilo[13];
    char aux[15];

    /* ------------------------- Memorias compartidas ------------------------- */

    /** 
     * Memoria compartida para asignar llaves de comunicación al cliente
     * nuevo para que pueda comunicarse con el hilo que le es asignado
     */
    parametrosCliente = (int *)shm(sizeof(int)*2, "shm_claves", 'v');

    /* ------------------------------ Semáforos ------------------------------- */
    
    /* Semáforo del servidor */
    semServidor = semnew(1, "sem_servidor", 'w');

    /* Semáforo del cliente */
    semCliente = semnew(0, "sem_cliente", 'x');

    /* Semáforo de claves */
    semClaves = semnew(0, "sem_claves", 'y');

    /* --------------------------- Configuraciones ---------------------------- */

    /* Llenado del arreglo con las llaves de conexión para la memoria compartida con hilos */
    for (contador = 0; contador < MAX_CLIENTES; contador++) {
        llaves_conexion[contador][0] = contador + 97;
        llaves_conexion[contador][1] = true;
    }

    /* Hilos para atender a los clientes */
    pthread_attr_t atributos;
	pthread_t hilos[MAX_CLIENTES];
	pthread_attr_init(&atributos);
	pthread_attr_setdetachstate(&atributos, PTHREAD_CREATE_DETACHED);

    /* Reinicio del contador de número de archivo */
    contador = 0;
    initscr();//Inicia el display
    printw("Presione ctrl + c para salir en cualquier momento\n");
    refresh();
    while (true){
        /* Obtención de las claves */
        llave = llaves_conexion[contador][0];
        archivo = contador+300;

        /* La llave asignada ya no está disponible */
        llaves_conexion[contador][1] = false;

        /* Conversión del número de archivo asignado a cadena */
        sprintf(numero_archivo, "%d", archivo);

        /* Memoria compartida del hilo asignado */
        strcpy(archivo_shm, "shm_");
        strcat(archivo_shm, numero_archivo);
        shm(sizeof(int)*2, archivo_shm, llave);

        /* Memoria compartida del hilo asignado String values*/
        strcpy(aux, archivo_shm);
        strcat(aux, "_str");
        shm(sizeof(char)*MAX_SHM_STRING, aux, llave + 200);

        /* Semáforo del hilo */
        strcpy(archivo_sem_hilo, "sem_hilo_");
        strcat(archivo_sem_hilo, numero_archivo);
        semnew(0, archivo_sem_hilo, llave+400);

        /* Semáforo del cliente */
        strcpy(archivo_sem_cliente, "sem_cliente_");
        strcat(archivo_sem_cliente, numero_archivo);
        semnew(0, archivo_sem_cliente, llave+500);

        down(semClaves, 0);

        /* Envío de las claves al cliente */
        parametrosCliente[0] = llave;
        parametrosCliente[1] = archivo;

        up(semCliente, 0);
        down(semClaves, 0);

        /* Creación del hilo asignado */
        pthread_create(&hilos[contador], &atributos, (void *)serveCustomer, (void *)parametrosCliente);

        up(semCliente, 0);

        contador++;

    }
    endwin();
    return 0;
}

void serveCustomer(void *arguments){
    pthread_t id = pthread_self();
    int *claves_comunicacion_cliente = (int *)arguments;
    int llave, archivo;
    int *chat, *asientos;
    char *chatStr, aux[15];
    int sem_hilo, sem_cliente;
    char numero_archivo[4], archivo_shm[8], archivo_sem_cliente[16], archivo_sem_hilo[13];
    char numero_asiento[3], archivo_sem_asiento[15];
    bool otraCompra = true, login = true;

    /* Obtención de las claves del servidor */
    llave = claves_comunicacion_cliente[0];
    archivo = claves_comunicacion_cliente[1];

    /* ------------------- Obtención de nombres de archivos ------------------- */

    /* Conversión del número de archivo asignado a cadena */
    sprintf(numero_archivo, "%d", archivo);

    /* Archivo: shm_xxx */
    strcpy(archivo_shm, "shm_");
    strcat(archivo_shm, numero_archivo);

    /* Archivo: shm_xxx_str*/
    strcpy(aux, archivo_shm);
    strcat(aux, "_str");

    /* Archivo: sem_cliente_xxx */
    strcpy(archivo_sem_cliente, "sem_cliente_");
    strcat(archivo_sem_cliente, numero_archivo);

    /* Archivo: sem_hilo_xxx */
    strcpy(archivo_sem_hilo, "sem_hilo_");
    strcat(archivo_sem_hilo, numero_archivo);

    /* ------------------------- Memorias compartidas ------------------------- */

    /* Chat */
    chatStr = (char *)shm(sizeof(char)*MAX_SHM_STRING, aux, llave + 200);
    chat = (int *)shm(sizeof(int)*2, archivo_shm, llave);

    /* Semáforo del hilo */
    sem_hilo = semnew(0, archivo_sem_hilo, llave+400);

    /* Semáforo del cliente */
    sem_cliente = semnew(0, archivo_sem_cliente, llave+500);

    /* ------------------------ Inicio de comunicación ------------------------ */

    printw("[Hilo %ld] Nueva conexión.\n", id);
    refresh();
    while (login){
        otraCompra = true;
        down(sem_hilo, 0);//Se ocupa el semaforo hilo
        chat[1] = validUser(chatStr);
        printw("[Hilo %ld] El usuario se logueo de manera %s\n", id,(chat[1])?"correcta":"fallida");
        refresh();
        up(sem_cliente, 0);
        if(!chat[1]){
            down(sem_hilo, 0);
            printw("[Hilo %ld]El usuario se registro %s\n", id, (saveUser(chatStr))?"correctamente":"erroneamente");
            refresh();
        }
        //Control 2
        while (otraCompra){
            // up(sem_cliente, 0);//Desocupamos al cliente
            down(sem_hilo, 0);//Esperamos al cliente
            switch (chat[0]){
                case 1: //VER CATALOGO - listo
                    showInventory(chatStr);
                    up(sem_cliente, 0);//Desocupamos al cliente
                    printw("[Hilo %ld] Mostrando catalogo al usuario \n", id);
                    break;
                case 2: // VER CARRITO
                    char user[20];
                    strcpy(user,chatStr);
                    showCarrito(user, chatStr);
                    up(sem_cliente,0);
                    printw("[Hilo %ld] Mostrando carrito al usuario \n", id);
                    refresh();
                    if(strlen(chatStr) > 1){
                        down(sem_hilo, 0);//Esperamos al cliente
                        int opcDelete = chat[1];
                        up(sem_cliente, 0);
                        down(sem_hilo, 0);
                        if(chat[1] == true){
                            printw("[Hilo %ld] El usuario quitara un libro del carrito \n", id);
                            if(delCarrito(user, chatStr ))
                                printw("[Hilo %ld] El usuario quito un libro de su carrito\n", id);
                            refresh();
                            up(sem_cliente, 0);
                        }
                        else
                            printw("[Hilo %ld] El usuario NO quito un libro de su carrito \n", id);
                    }
                    break;
                case 3: //COMPRAR CARRITO
                    printw("[Hilo %ld] El usuario esta pensando en agregar o comprar carrito \n", id);
                    refresh();
                    char userx[20];
                    strcpy(userx, chatStr);
                    up(sem_cliente, 0);
                    down(sem_hilo, 0);
                    if(chat[1] == true){ //Agregar carrito
                        down(sem_hilo, 0);
                        printw("[Hilo %ld] El usuario agrego un libro de manera %s\n", id, (addCarrito(userx, chatStr)?"correcta":"erronea"));
                        up(sem_cliente, 0);
                    }
                    down(sem_hilo, 0);
                    if(chat[1] == true){//compramos
                        printw("[Hilo %ld] El usuario compro los productos de manera %s\n", id, (buyCarrito(userx, chatStr)?"correcta":"erronea"));
                        up(sem_cliente,0);
                    }
                    break;
                case 4: // LOG OUT
                    otraCompra = false;
                    up(sem_cliente, 0);
                    printw("[Hilo %ld] El usuario salio de sesion\n", id);
                    break;
                case 5: //SALIR TOTALMENTE - 
                    otraCompra = false;
                    login = false;
                    up(sem_cliente, 0);
                    printw("[Hilo %ld] El usuario salio de la aplicacion\n", id);
                    break;
                default:
                    up(sem_cliente, 0);
                    printw("[Hilo %ld] El usuario hizo una seleccion no valida\n", id);
                    break;
            }
            refresh();
            //WHILE2
        }
        
        // up(sem_cliente, 0);//Desocupamos al cliente
        // down(sem_hilo, 0);//Esperamos al cliente

        //WHILE 1
        refresh();
    }

    //HILO
    /* Si el usuario no requiere realizar otra compra */
    printw("[Hilo %ld] Conexión terminada.\n", id);
    refresh();
    pthread_exit(NULL);
}

bool validUser(char *credentials){
    char user[20], pass[20], buffer[150];
    char *credenciales = strtok(credentials, ","); //SPLIT FOR C
    strcpy(user, credenciales);
    credenciales = strtok(NULL, " ");
    strcpy(pass, credenciales);
    bool foundIt = false;
    FILE *fc = fopen("Usuarios.dat", "rb");
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        char userF[30] = "", passF[20] = "";
        /*Se suma la logitud del substring "IDU:" al igual que los demas*/
        //char *ID = strstr(buffer, "IDU:") + 4, *IDEND = strstr(buffer, ":IDUEND");
        //char *NAME = strstr(buffer, "NAMEU:") + 6, *NAMEEND = strstr(buffer, ":NAMEUEND");
        char *USER = strstr(buffer, "USER:") + 5, *USEREND = strstr(buffer, ":USEREND");
        char *PASS = strstr(buffer, "PASSU:") + 6, *PASSEND = strstr(buffer, ":PASSUEND");
        // if(ID != NULL && IDEND!=NULL)
        //     strncpy(id, ID, strlen(ID) - strlen(IDEND));
        // if(NAME != NULL && NAMEEND != NULL)
        //     strncpy(nameF, NAME, strlen(NAME) - strlen(NAMEEND));
        if(USER != NULL && USEREND != NULL)
            strncpy(userF, USER, strlen(USER) - strlen(USEREND));
        if(PASS != NULL && PASSEND != NULL)
            strncpy(passF, PASS, strlen(PASS) - strlen(PASSEND));
        if(strcmp(user, userF) == 0 && strcmp(pass, passF) == 0){ foundIt = true; break;}
    }
    fclose(fc);
    __fpurge(stdin);
    return foundIt;
}

bool saveUser(char *credentials){
    char name[40], user[40], pass[40];
    char *credenciales = strtok(credentials, ","); //SPLIT FOR C
    strcpy(user, credenciales);
    credenciales = strtok(NULL, " ");
    credenciales = strtok(credenciales, ",");
    strcpy(name, credenciales);
    credenciales = strtok(NULL, " ");
    strcpy(pass, credenciales);
    FILE *fc = fopen("Usuarios.dat", "a");
    if (fc == NULL) { printw("Error al leer el archivo"); return false; }
    fprintf(fc, "IDU:%i:IDUEND,NAMEU:%s:NAMEUEND,USER:%s:USEREND,PASSU:%s:PASSUEND\n", lenghFile("Usuarios.dat") + 1, name, user, pass);
    fclose(fc);
    __fpurge(stdin);
    return true;
}

void showInventory(char *comunicator){
    memset(comunicator, 0, MAX_SHM_STRING);
    char buffer[150];
    FILE *fc = fopen("Libros.dat", "rb");
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        char idF[20] = "", nameF[30] = "", inventaryF[20] = "";
        /*Se suma la logitud del substring "IDU:" al igual que los demas*/
        char *IDL = strstr(buffer, "IDL:") + 4, *IDLEND = strstr(buffer, ":IDLEND");
        char *NAMEL = strstr(buffer, "NAMEL:") + 6, *NAMELEND = strstr(buffer, ":NAMELEND");
        char *INVENTORY = strstr(buffer, "INVENTORY:") + 10, *INVENTORYEND = strstr(buffer, ":INVENTORYEND");
        if(IDL != NULL && IDLEND!=NULL)
            strncpy(idF, IDL, strlen(IDL) - strlen(IDLEND));
        if(NAMEL != NULL && NAMELEND != NULL)
            strncpy(nameF, NAMEL, strlen(NAMEL) - strlen(NAMELEND));
        if(INVENTORY != NULL && INVENTORYEND != NULL)
            strncpy(inventaryF, INVENTORY, strlen(INVENTORY) - strlen(INVENTORYEND));
        strcat(comunicator, idF); strcat(comunicator, "    ");
        strcat(comunicator, nameF); strcat(comunicator, "    ");
        strcat(comunicator, inventaryF); strcat(comunicator, "\n");
    }
    fclose(fc);
    __fpurge(stdin);
}

void getBook(char id[], char data[]){
    strcpy(data, "");
    char buffer[150];
    FILE *fc = fopen("Libros.dat", "rb");
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        char idF[20] = "", nameF[30] = "", inventaryF[20] = "";
        /*Se suma la logitud del substring "IDU:" al igual que los demas*/
        char *IDL = strstr(buffer, "IDL:") + 4, *IDLEND = strstr(buffer, ":IDLEND");
        char *NAMEL = strstr(buffer, "NAMEL:") + 6, *NAMELEND = strstr(buffer, ":NAMELEND");
        char *INVENTORY = strstr(buffer, "INVENTORY:") + 10, *INVENTORYEND = strstr(buffer, ":INVENTORYEND");
        if(IDL != NULL && IDLEND!=NULL)
            strncpy(idF, IDL, strlen(IDL) - strlen(IDLEND));
        if(NAMEL != NULL && NAMELEND != NULL)
            strncpy(nameF, NAMEL, strlen(NAMEL) - strlen(NAMELEND));
        if(INVENTORY != NULL && INVENTORYEND != NULL)
            strncpy(inventaryF, INVENTORY, strlen(INVENTORY) - strlen(INVENTORYEND));
        if(strcmp(id, idF) == 0){
            strcpy(data, id); strcat(data, ",");
            strcat(data, nameF); strcat(data, ",");
            strcat(data, inventaryF);
            break;
        }
    }
    fclose(fc);
    __fpurge(stdin);
}

void showCarrito(char user[], char *comunicator){
    memset(comunicator, 0, MAX_SHM_STRING);
    char carrito[30], buffer[150], data[50], aux2[20];
    char *aux = aux2;
    strcpy(carrito, "Carrito_"); strcat(carrito, user);
    strcat(carrito, ".dat");
    FILE *fc = fopen(carrito, "r");
    if (fc == NULL) { fc = fopen(carrito, "w"); strcpy(comunicator, "\n"); return ;}
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        removerCaracteres(buffer, "\n");
        getBook(buffer, data);
        aux = strtok(data, ",");
        strcat(comunicator, aux);
        aux = strtok(NULL, "");
        aux = strtok(aux, ",");
        strcat(comunicator, "\t");  
        strcat(comunicator, aux);
        strcat(comunicator, "\t");strcat(comunicator ,"1\n");
    }
    fclose(fc);
    __fpurge(stdin);
}

bool addCarrito(char user[], char *comunicator){
    char carrito[30], buffer[150], data[50];
    strcpy(carrito, "Carrito_"); strcat(carrito, user);strcat(carrito, ".dat");
    if(lenghFile(carrito) >= 3){
        strcpy(comunicator, "No puedes agregar mas de 3 libros\n");
        return false;
    }
    getBook(comunicator, data);
    if(strlen(data) <= 1){
        strcpy(comunicator, "No existe ese libro\n");
        return false;
    }
    if(getInventory(comunicator) < 1){
        strcpy(comunicator, "El libro no cuenta con existencias\n");
        return false;
    }
    FILE *fc = fopen(carrito, "a");
    while (fgets(buffer, 150, fc) != NULL){
        removerCaracteres(buffer, "\n");
        if(strcmp(buffer, comunicator) == 0){
            strcpy(comunicator ,"El libro ya esta en tu carrito\n");
            return false;
        }
    }
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    fprintf(fc, "%s\n", comunicator);
    strcpy(comunicator, "Se agrego correctamente a tu carrito\n");
    fclose(fc);
    __fpurge(stdin);
    return true;
}

bool delCarrito(char user[], char *book){
    char buffer[150], carrito[30], id[10], data[20] = "";
    strcpy(id, book); strcat(id, "\n");
    int line = 0;
    //bool foundIt = false;
    strcpy(carrito, "Carrito_"); strcat(carrito, user);
    strcat(carrito, ".dat");
    FILE *fc = fopen(carrito, "r");
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        if(strcmp(id, buffer) != 0){
            strcat(data, buffer);
        }
    }
    fclose(fc);
    FILE *fc2 = fopen(carrito, "w");
    fprintf(fc2, "%s", data);
    fclose(fc2);
    __fpurge(stdin);
    return true;
}

bool buyCarrito(char user[], char *comunicator){
    char buffer[150], carrito[30];
    strcpy(carrito, "Carrito_"); strcat(carrito, user);strcat(carrito, ".dat");
    if(lenghFile(carrito) == 0){
        strcpy(comunicator ,"No tiene nada en el carrito\n");
        return false;
    }
    FILE *fc = fopen(carrito, "r");
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        removerCaracteres(buffer, "\n");
        if(getInventory(buffer) == 0){
            strcpy(comunicator, "El libro no cuenta con existencias\n");
            delCarrito(user, buffer);
            return false;
        }
        reduceInventory(buffer);
    }
    fclose(fc);
    FILE *fc2 = fopen(carrito, "w");
    fprintf(fc2, "%s", "");
    fclose(fc2);
    __fpurge(stdin);
    strcpy(comunicator, "Se compro todo con exito\n");
    return true;
}

bool reduceInventory(char id[]){
    char buffer[150], data[500] = "";
    FILE *fc = fopen("Libros.dat", "r");
    if (fc == NULL) { printw("Error al leer el archivo"); exit(1); }
    while (fgets(buffer, 150, fc) != NULL){ //BY LINE
        char idF[20] = "", nameF[30] = "", inventaryF[20] = "";
        /*Se suma la logitud del substring "IDU:" al igual que los demas*/
        char *IDL = strstr(buffer, "IDL:") + 4, *IDLEND = strstr(buffer, ":IDLEND");
        char *NAMEL = strstr(buffer, "NAMEL:") + 6, *NAMELEND = strstr(buffer, ":NAMELEND");
        char *INVENTORY = strstr(buffer, "INVENTORY:") + 10, *INVENTORYEND = strstr(buffer, ":INVENTORYEND");
        if(IDL != NULL && IDLEND!=NULL)
            strncpy(idF, IDL, strlen(IDL) - strlen(IDLEND));
        if(NAMEL != NULL && NAMELEND != NULL)
            strncpy(nameF, NAMEL, strlen(NAMEL) - strlen(NAMELEND));
        if(INVENTORY != NULL && INVENTORYEND != NULL)
            strncpy(inventaryF, INVENTORY, strlen(INVENTORY) - strlen(INVENTORYEND));
        strcat(data, "IDL:");strcat(data, idF); strcat(data,":IDLEND");
        strcat(data, ",NAMEL:");strcat(data, nameF); strcat(data,":NAMELEND");
        if(strcmp(id, idF) == 0){
            int x = atoi(inventaryF);
            x = x-1;
            sprintf(inventaryF, "%i", x);
        }
        strcat(data, ",INVENTORY:");strcat(data, inventaryF); strcat(data,":INVENTORYEND\n");
    }
    fclose(fc);
    FILE *fc2 = fopen("Libros.dat", "w");
    fprintf(fc2, "%s", data);
    fclose(fc2);
    __fpurge(stdin);
    return true;
}

int getInventory(char id[]){
    char data[50], aux2[20], aux3[5];
    char *aux = aux2;
    getBook(id, data);
    aux = strtok(data, ","); aux = strtok(NULL, "");
    aux = strtok(aux, ","); aux = strtok(NULL, "");
    strcpy(aux3, aux);
    return atoi(aux3);
}

int lenghFile(char nameFc[]){
    FILE *fc = fopen(nameFc, "r");
    char ca;
    int cont = 0;
    while(true){
        ca = fgetc(fc);
        if(ca == '\n'){
            cont++;
        }
        if(ca == EOF){
            break;
        }
    }
    fclose(fc);
    return cont;
}

void removerCaracteres(char *cadena, char *caracteres) {
    int indiceCadena = 0, indiceCadenaLimpia = 0;
    int deberiaAgregarCaracter = 1;
    while (cadena[indiceCadena]) {
        deberiaAgregarCaracter = 1;
        int indiceCaracteres = 0;
        while (caracteres[indiceCaracteres]) {
            if (cadena[indiceCadena] == caracteres[indiceCaracteres]) {
                deberiaAgregarCaracter = 0;
            }
            indiceCaracteres++;
        }
        if (deberiaAgregarCaracter) {
            cadena[indiceCadenaLimpia] = cadena[indiceCadena];
            indiceCadenaLimpia++;
        }
        indiceCadena++;
    }
    cadena[indiceCadenaLimpia] = 0;
}