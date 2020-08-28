#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include <math.h>
#include <pthread.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <signal.h>
#include <semaphore.h>


int main(void){
    key_t claveGlobal = ftok("/bin/man",35);
    int shmid = shmget(claveGlobal,sizeof(int),IPC_CREAT | 0660);

    int opcion;

    do
    {
        printf( "\n   1. Datos de los sensores.");
        printf( "\n   2. Reglas que se cumplen y estado de la alarma.");
        printf( "\n   3. Información de diagnóstico de sensores.");
        printf( "\n   4. Salir." );
        printf( "\n\n   Introduzca opción: ");

        scanf( "%d", &opcion );

        /* Inicio del anidamiento */

        switch ( opcion )
        {
            case 1: printf( "\n ");
                    break;

            case 2: printf( "\n");
                    break;

            case 3: printf( "\nActivo/Inactivo\tPID\tFecha de último dato recibido");
                    break;
         }

         /* Fin del anidamiento */

    } while ( opcion != 4 );

    return 0;
}