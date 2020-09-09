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
#define SHMSZ 4   /* Constante para la memoria compartida */

typedef struct
{              /* Estructura Arreglo dinámico de enteros */
  int *array;  /* Puntero a un entero */
  size_t used; /* Cantidad de datos llenados */
  size_t size; /* Cantidad de datos disponibles a llenar */
} Array;

typedef struct Sensor
{                 /* Estructura Sensor */
  int id;         /* Id del sensor */
  int tipoS;      /* Tipo del sensor */
  int th;         /* Threshold del sensor */
  int activo;     /* Dato que abarca 1 o 0 que determina si el sensor se encuentra activo o no */
  int comm;       /* Puerto de comunicación con ./sensorx */
  int shared_comm;/* Puerto de comunicación con ./monitoreo */
  Array lecturas; /* Lista de lecturas */
} Sensor_t;

typedef struct nodoSensor
{                               /* Estructura de nodo que almacena Sensor */
  Sensor_t sensor;              /* Sensor perteneciente al nodo */
  struct nodoSensor *siguiente; /* Puntero al siguiente nodo*/
} nodoSensor_t;

void initArray(Array *, size_t); /* Función que inicializa el arreglo dinámico */

void insertArray(Array *, int); /* Función que realiza la inserción de datos al arreglo dinámico */

void freeArray(Array *); /* Función que libera el espacio de memoria usado por el arreglo dinámico */

bool contains(Array *, int); /* Función que determina si dicho elemento se encuentra en el arreglo dinámico */

void clearScreen();

FILE *archivoCSV;                    /* Puntero al archivo csv */
const char s[2] = ",";               /* Separador ',' de las líneas en el archivo csv */
int cantSensores = 0;
char enunciadoSensores[MAXSTR][MAXSTR];
Array listaClaves;
int main(void){
    
    /*------------------------------------------------------------*/
    
    initArray(&listaClaves, 1);
    
    //int shmid = shmget(claveGlobal,sizeof(int),IPC_CREAT | 0660);
    archivoCSV = fopen("sensores.csv", "r"); /* Leemos el archivo csv */
    
    char content[MAXSTR];
    
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
      strcpy(tipo, "coop");
    }
    else {
      strcpy(tipo, "comp");
    }
    sprintf(enunciadoSensores[cantSensores],"Sensor %s %d",tipo,id);
    insertArray(&listaClaves, comm);
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
                    while (1){
                      clearScreen();
                      for(int i=0;i< cantSensores; i++){
                        printf("%s\t",enunciadoSensores[i]);
                      }
                      printf( "\n ");
                      for(int i=0;i< cantSensores; i++){
                        int clave = shmget(listaClaves.array[i], SHMSZ,  0666);
                        int *shm = shmat(clave, NULL, 0);
                        printf("%d\t\t",*shm);
                      }
                      printf( "\n ");
                      usleep(1000000); 
                    }
                    break;

            case 2: printf( "\n");
                    key_t claveGlobalValores = ftok("/bin/man",35);
                    int shmidClaveGlobalValores = shmget(claveGlobalValores,sizeof(Array),0666);
                    Array *valClaveGlobalValores = (Array *)shmat(shmidClaveGlobalValores, 0, 0);
                    Array memoriasCompartidas = *valClaveGlobalValores;
                    while (1){
                      clearScreen();
                      int resp = 1;
                      for(int k = 0;k< memoriasCompartidas.used; k++){
                        int clave = shmget(memoriasCompartidas.array[k], SHMSZ,  0666);
                        int *shm = shmat(clave, NULL, 0);
                        int val = *shm;
                        if(k==0){
                          if(val){
                            printf("S_coop es mayor a 0.7\n");
                          }
                          else{
                            printf("S_coop es menor a 0.7\n");
                          }
                        }else{
                          if(val){
                            printf("Sensor competitivo cumple la condición\n");
                          }
                          else{
                            printf("Sensor competitivo no cumple la condición\n");
                          }
                        }
                        resp=resp*val;
                      }
                      if(resp){
                        printf("Alarma encendida\n");
                        }
                      else{
                        printf("Alarma apagada\n");
                      }
                      usleep(1000000);
                    }
                    break;

            case 3: printf( "\n");
                    key_t claveGlobalSensores = ftok("/bin/man",45);
                    int shmidClaveGlobalSensores = shmget(claveGlobalSensores,sizeof(Array),IPC_CREAT | 0660);
                    Array *valClaveGlobalSensores = (Array *)shmat(shmidClaveGlobalSensores, 0, 0);
                    Array memoriasSensor = *valClaveGlobalSensores;
                    while (1){
                      clearScreen();
                      printf( "Activo/Inactivo\tPID\tFecha de último dato recibido\n");
                      for(int x=0;x<memoriasSensor.used;x++){
                        int clave = shmget(memoriasSensor.array[x], SHMSZ,  0666);
                        Sensor_t *shm = (Sensor_t *)shmat(clave, NULL, 0);
                        Sensor_t sensor = *shm;
                        if(sensor.activo){
                          printf("Activo\t");
                        }else{
                          printf("Inactivo\t");
                        }
                      }
                      usleep(1000000);
                    }
                    break;
         }

         /* Fin del anidamiento */

    } while ( opcion != 4 );

    return 0;
}

void initArray(Array *a, size_t initialSize)
{
  a->array = malloc(initialSize * sizeof(int)); /* Se inicia por malloc el espacio de memoria para el array */
  a->used = 0;                                  /* Se inicializa en 0 */
  a->size = initialSize;
}

void insertArray(Array *a, int element)
{
  // a->used is the number of used entries, because a->array[a->used++] updates a->used only *after* the array has been accessed.
  // Therefore a->used can go up to a->size
  if (a->used == a->size)
  {
    a->size *= 2;
    a->array = realloc(a->array, a->size * sizeof(int));
  }
  a->array[a->used++] = element;
}

void freeArray(Array *a)
{
  free(a->array);        /* Se libera el espacio en memoria del array */
  a->array = NULL;       /* Array apunta a Null */
  a->used = a->size = 0; /* Tanto el size como el used pasan a 0 */
}

bool contains(Array *a, int element)
{
  for (int i = 0; i < a->used; i++)
  { /* Se itera por número de elementos existentes en el arreglo */
    if (a->array[i] == element)
    { /* Si el elemento se encuentra devuelve true */
      return true;
    }
  }
  return false; /* Se devuelve false ya que no se lo encontró */
}

void *thread_function(void *data)
{ /* Función ejecutada por el hilo que recibe un nodoSensor */
  int comm = *(int *)data;
  int shmid, *shm; /* Se crean las variables para la memoria compartida */
  if ((shmid = shmget(comm, SHMSZ, 0666)) < 0)
  { /* Se verifica la conexión con el puerto */
    perror("shmget");
  }
  if ((shm = shmat(shmid, NULL, 0)) == (int *)-1)
  { /* Se obtiene puntero de int del valor compartido */
    perror("shmat");
  }
  int val; /* Se crea variable para evitar repetición de datos sucesivos */
  while (1)
  {
    if (*shm != val)
    { /* Si el valor de la memoria compartida es un valor nuevo */
      val = *shm; /* Se guarda en val el valor del puntero 'shm' */
    }
  }
}

void clearScreen()
{
  const char *CLEAR_SCREEN_ANSI = "\e[1;1H\e[2J";
  write(STDOUT_FILENO, CLEAR_SCREEN_ANSI, 12);
}