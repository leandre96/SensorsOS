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

#define MAXSTR 50 /* Cantidad de caracteres en la linea del archivo csv */

FILE *archivoCSV;                    /* Puntero al archivo csv */
const char s[2] = ",";               /* Separador ',' de las líneas en el archivo csv */
int cantSensores = 0;
char **enunciadoSensores;
int main(void){
    key_t claveGlobal = ftok("/bin/man",35);
    int shmid = shmget(claveGlobal,sizeof(int),IPC_CREAT | 0660);
    archivoCSV = fopen("sensores.csv", "r"); /* Leemos el archivo csv */
    while (fgets(content, MAXSTR, archivoCSV) != NULL){           /* Mientras haya una línea por leer en el archivo csv */
    content[strlen(content) - 1] = '\0'; /* Se elimina el salto de línea */
    char *tkn;                           /* Creamos un token para ejecutar el split */
    int id, tipoS, th, comm;             /* Se crean las variables que almacenaran los datos del sensor */
    tkn = strtok(content, s);            /* Se crea un iterador de la línea del archivo */
    int split = 1;                       /* Se crea una variable de referencia de la posición del cursor de tkn */
    while (tkn != NULL)
    { /* Se itera al token hasta que apunte a NULL */
      switch (split)
      {
      case 1:
        id = atoi(tkn); /* Se asigna el id del sensor */
        break;
      case 2:
        tipoS = atoi(tkn); /* Se asigna el tipo del sensor */
        break;
      case 3:
        th = atoi(tkn); /* Se asigna el th del sensor */
        break;
      case 4:
        comm = atoi(tkn); /* Se asigna el puerto de comunicación del sensor */
        break;
      }
      tkn = strtok(NULL, s); /* iteramos el token */
      split++;               /* Se aumenta el valor del cursor */
    }
    char tipo[MAXSTR];
    if (tipoS >= 5){
        tipo = "cooperativo";
    }
    else {
        tipo = "competitivo";
    }
    sprintf(enunciadoSensores[cantSensores],"Sensor %s %d :%d",tipo,id,th);
    cantSensores++;
  }
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