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
  Array lecturas; /* Lista de lecturas */
} Sensor_t;

typedef struct nodoSensor
{                               /* Estructura de nodo que almacena Sensor */
  Sensor_t sensor;              /* Sensor perteneciente al nodo */
  struct nodoSensor *siguiente; /* Puntero al siguiente nodo*/
} nodoSensor_t;

bool esCoop(int); /* Función que determina si es un sensor cooperativo o competitivo*/

Sensor_t crearSensor(int, int, int, int); /* Función que recibe los datos necesarios para la creación de una estructura Sensor, devuelve un puntero a dicha estructura */

void initArray(Array *, size_t); /* Función que inicializa el arreglo dinámico */

void insertArray(Array *, int); /* Función que realiza la inserción de datos al arreglo dinámico */

void freeArray(Array *); /* Función que libera el espacio de memoria usado por el arreglo dinámico */

bool contains(Array *, int); /* Función que determina si dicho elemento se encuentra en el arreglo dinámico */

void cambiarEstado(Sensor_t *); /* Función que cambia el estado de activo a inactivo y viceversa de un sensor */

bool aprobado(Sensor_t, int); /* Función que determina si dicha lectura es mayor al valor del th del Sensor */

float S_coop(nodoSensor_t *); /* Función que devuelve el cálculo del s_coop de la lista de sensores cooperativos */

float average(Array); /* Función que devuelve el promedio de los datos pertenecientes a un arreglo dinámico */

void inicializarLecturasSensor(Sensor_t *); /* Función que inicializa las lecturas del sensor */

void borrarLecturasSensor(Sensor_t *); /* Función que reinicializa las lecturas del sensor */

void anadirLecturaSensor(Sensor_t *, int); /* Función que añade una lectura a dicho sensor */

float varianza(Sensor_t); /* Función que calcula la varianza de un sensor */
int cantidadSensor(nodoSensor_t *);

void *thread_function(void *); /* Función ejecutada por un hilo */

bool nodoSensorAprobado(nodoSensor_t *); /* Función que determina la respuesta de un sensor competitivo si es válida o no */

nodoSensor_t *NodoPorMenorVarianza(nodoSensor_t *); /* Función de sensores competitivos que devuelve el sensor competitivo con la menor varianza */

nodoSensor_t *sensoresCompetitivos;  /* Lista de sensores competitivos */
nodoSensor_t *sensoresCooperativos;  /* Lista de sensores cooperativos */
int numSensoresCompetitivos = 0;     /* Cantidad de sensores competitivos */
int numSensoresCooperativos = 0;     /* Cantidad de sensores cooperativos */
Array categoriaSensoresCompetitivos; /* Arreglo dinámico de tipos de sensores competitivos */
FILE *archivoCSV;                    /* Puntero al archivo csv */
const char s[2] = ",";               /* Separador ',' de las líneas en el archivo csv */
double deltaT;                       /* Variable que almacena el deltaT ingresado por el usuario */
time_t start_t, end_t;               /* Variables time_t que nos ayudara a guardar 2 referencias de tiempo */
bool programaPrincipal = false;      /* Variable que identifica si se encuentra o no en el proceso padre */
int main(int argc, char *argv[])
{
  if (argc != 3)
  {                                                                          /* Si la cantidad de argumentos no es 3 */
    printf("Se requieren 2 argumentos: [deltaT] [nombre-de-archivo.csv]\n"); /* Se imprime información en pantalla */
    return 0;                                                                /* El programa termina su ejecución */
  }
  int pidCooperativo;              /* Se crea variable que almacena un pid */
  int pidCompetitivo;              /* Se crea variable que almacena un pid */
  char *eptr;                      /* Se crea puntero a char con fin de almacenamiento de double */
  deltaT = strtod(argv[1], &eptr); /* Se almacena el string convertido en double de deltaT */
  if (deltaT < 1)
  { /* Se válida que se un número el ingresado */
    printf("Ingrese un valor válido de deltaT\n");
    return 0;
  }
  archivoCSV = fopen(argv[2], "r"); /* Leemos el archivo csv */
  if (!archivoCSV)
  { /* Se válida que sea un archivo existente */
    printf("Ingrese un nombre de archivo válido\n");
    return 0;
  }
  printf("Creando sensores\n");
  char content[MAXSTR];                         /* Contenido de cada linea del archivo csv */
  initArray(&categoriaSensoresCompetitivos, 1); /* Inicio vector con los tipos de sensores competitivos */
  while (fgets(content, MAXSTR, archivoCSV) != NULL)
  {                                      /* Mientras haya una línea por leer en el archivo csv */
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
    Sensor_t sensor = crearSensor(id, tipoS, th, comm); /* Creo un sensor */
    if (esCoop(tipoS))
    {                                                                           /* Pregunto si es sensor del tipo cooperativo */
      nodoSensor_t *nodo_sensor = (nodoSensor_t *)malloc(sizeof(nodoSensor_t)); /* Espacio de memoria del nodo sensor */
      nodo_sensor->sensor = sensor;                                             /* Se asigna el valor del puntero de sensor al nodoSensor */
      nodo_sensor->siguiente = sensoresCooperativos;                            /* Se enlazan los nodos */
      sensoresCooperativos = nodo_sensor;                                       /* Asignacion a la lista de cooperativos */
      numSensoresCooperativos++;                                                /* Aumento la cantidad de sensores cooperativos */
    }
    else
    {
      if (!contains(&categoriaSensoresCompetitivos, tipoS))
      { /* Si no contiene dicho tipo de sensor competitivo agrega el tipo a una lista */
        insertArray(&categoriaSensoresCompetitivos, tipoS);
      }
      nodoSensor_t *nodo_sensor = (nodoSensor_t *)malloc(sizeof(nodoSensor_t)); /* Espacio de memoria del nodo sensor */
      nodo_sensor->sensor = sensor;                                             /* Se asigna el valor del puntero de sensor al nodoSensor */
      nodo_sensor->siguiente = sensoresCompetitivos;                            /* Se enlazan los nodos */
      sensoresCompetitivos = nodo_sensor;
      numSensoresCompetitivos++; /* Aumento la cantidad de sensores competitivos */
    }
  }
  printf("Creados sensores\n");
  printf("Sensores cooperativos %d \n", numSensoresCooperativos);
  printf("Sensores competitivos %d \n", numSensoresCompetitivos);
  nodoSensor_t *iterador;                                      /* Se crea un iterador  */
  const int numPipes = categoriaSensoresCompetitivos.used + 1; /* Dicha variable guarda la cantidad de pipes a crearse */
  int *memoriasCompartidasAcceso[numPipes], *memoriasCompartidasValor[numPipes];
  sem_t semAcceso[numPipes], semValor[numPipes];
  for(int k = 0; k < numPipes ; k++ ){
    key_t claveA = ftok("/bin/ls",33+k);
    key_t claveV = ftok("/bin/ls",33-k);
    int shmidA = shmget(claveA,sizeof(int),IPC_CREAT | 0660);
    int shmidV = shmget(claveV,sizeof(int),IPC_CREAT | 0660);
    memoriasCompartidasAcceso[k] = (int *)shmat(shmidA, 0, 0);
    *memoriasCompartidasAcceso[k] = 0;
    sem_init(&semAcceso[k],0,1);
    sem_init(&semValor[k],0,1);
    memoriasCompartidasValor[k] = (int *)shmat(shmidV, 0, 0);
    *memoriasCompartidasValor[k] = 0;
  }
  ///Nuevo - Fin
  printf("Memorias compartidas creados\n");
  time(&start_t);          /* Se registra el tiempo actual */
  pidCooperativo = fork(); /* Se crea el proceso que se encarga de administrar los sensores cooperativos */
  if (pidCooperativo > 0)
  { /* En el proceso padre */
    printf("Creado proceso para sensores cooperativos \n");
    /* ------------------------------ PROCESO PADRE - CREACIÓN DE PROCESOS PARA LOS SENSORES COMPETITIVOS ------------------------------ */
    printf("Categorias de sensores competitivos %ld \n", categoriaSensoresCompetitivos.used);
    for (int j = 0; j < categoriaSensoresCompetitivos.used; j++)
    {                                   /* Se itera por la cantidad de tipos de sensores competitivos */
      nodoSensor_t *lista;              /* Creo una nueva lista de sensores */
      nodoSensor_t *iterador2;          /* Creo un iterador de lista de sensores */
      iterador2 = sensoresCompetitivos; /* Se asigna el iterador a la lista de sensores */
      while (iterador2 != NULL)
      {                                      /* Mientras el sensor no apunte a Null */
        Sensor_t sensor = iterador2->sensor; /* Se obtiene el sensor */
        if (sensor.tipoS == categoriaSensoresCompetitivos.array[j])
        {                                                                           /* Si el sensor pertence al tipoS seleccionado */
          nodoSensor_t *nodo_sensor = (nodoSensor_t *)malloc(sizeof(nodoSensor_t)); /* Espacio de memoria del nodo sensor */
          nodo_sensor->sensor = sensor;                                             /* Se asigna el valor del puntero de sensor al nodoSensor */
          nodo_sensor->siguiente = lista;                                           /* Se enlazan los nodos */
          lista = nodo_sensor;
        }
        iterador2 = iterador2->siguiente; /* Se pasa al siguiente nodoSensor */
      }

      int currentPipeIndex = j + 1;    /* Se crea la variable que almacena en que posición del arreglo de pipes se encontrará */
      if ((pidCompetitivo = fork()) == 0)
      {
        /* Aqui se ejecutan los sensores competitivos */
        iterador = lista;                              /* El iterador apunta a la nueva lista de sensores por categoría */
        const int numeroHilos = cantidadSensor(lista); /* Se obtiene la cantidad de hilos de acuerdo a la lista */
        printf("Numero de hilos del proceso %d para los sensores competitivos son %d \n", getpid(), numeroHilos);
        pthread_t thread_id[numeroHilos]; /* Se crea un arreglo de pthreads de acuerdo al numero de hilos */
        int ite;                          /* Se crea una variable para iterar */
        for (ite = 0; ite < numeroHilos; ite++)
        {        
          inicializarLecturasSensor(&(iterador->sensor));                                                           /* Se itera por la cantidad de hilos a usarse */
          pthread_create(&thread_id[ite], NULL, thread_function, iterador); /* Se crea el hilo asignando la función a ejecutarse con su propio parámetro */
          iterador = iterador->siguiente;                                   /* Se pasa al siguiente nodoSensor */
        }
        /* Aqui se maneja el hilo principal */
        while (1)
        {
          sem_wait(&semAcceso[currentPipeIndex]);
          int acc = *memoriasCompartidasAcceso[currentPipeIndex];
          sem_post(&semAcceso[currentPipeIndex]);
          if (acc)
          {
            bool respuesta;
            if (numeroHilos == 1)
            {                                        /* Si el numero de hilos creados es 1 */
              respuesta = nodoSensorAprobado(lista); /* Se determina la validez de dicho sensor */
            }
            else
            {                                                              /* Si hay más de un hilo */
              respuesta = nodoSensorAprobado(NodoPorMenorVarianza(lista)); /* Se obtiene la validez del sensor con menor varianza */
            }
            sem_wait(&semValor[currentPipeIndex]);
            *memoriasCompartidasValor[currentPipeIndex] = (int) respuesta;
            sem_post(&semValor[currentPipeIndex]);
          }
        }
      }
      else if (pidCompetitivo < 0)
      {
        /* -----------------------------------FALLIDA CREACIÓN DE PROCESOS---------------------------------------------- */
        printf("No se pudo crear el proceso\n");
        programaPrincipal = true; /* Se pone en true para entrar a otro bloque de ejecución */
        /* ------------------------------------------------------------------------------------------------------------- */
      }
      else
      {
        /* Proceso padre */
        lista = NULL;
        iterador2 = NULL;
        printf("Creado proceso para sensores competitivos \n");
      }
    }
    /* ---------------------------------------------------------------------------------------------------------------------------- */
  }
  else if (pidCooperativo < 0)
  {
    printf("No se pudo crear el proceso\n");
  }
  else
  {
    /* Aqui se ejecutan los sensores cooperativos */
    iterador = sensoresCooperativos;                 /* El iterador se asigna a la lista de sensores cooperativos */
    const int numeroHilos = numSensoresCooperativos; /* Se obtiene la cantidad de sensores cooperativos */
    pthread_t thread_id[numeroHilos];                /* Se crea un arreglo de pthreads de acuerdo al numero de hilos */
    printf("Numero de hilos del proceso %d para los sensores cooperativos son %d \n", getpid(), numeroHilos);
    int ite; /* Variable iteradora */
    for (ite = 0; ite < numeroHilos; ite++)
    {                                                                   /* Se itera por número de hilos */
      inicializarLecturasSensor(&(iterador->sensor));                   /* Se inicializa las lecturas de dicho sensor */
      pthread_create(&thread_id[ite], NULL, thread_function, iterador); /* Se crea el hilo */
      iterador = iterador->siguiente;                                   /* Se pasa al siguiente nodoSensor */
    }
    /* Aqui se maneja el hilo principal */
    while (1)
    {
      sem_wait(&semAcceso[0]);
      int acc = *memoriasCompartidasAcceso[0];
      sem_post(&semAcceso[0]);
      if (acc)
      {
        float val = S_coop(sensoresCooperativos); /* Se obtiene el s_coop de los sensores cooperativos */
        bool respuesta = (val > 0.7);             /* Si s_coop es mayor a 0.7 entonces se devuelve true */
        sem_wait(&semValor[0]);
        *memoriasCompartidasValor[0] = (int) respuesta;
        sem_post(&semValor[0]);
      }
    }
  }
  /* Aqui se ejecutan el programa principal */
  freeArray(&categoriaSensoresCompetitivos);
  while (1)
  {
    time(&end_t);          /* Se asigna el valor del tiempo actual */
    if (deltaT == difftime(end_t, start_t))
    { /* Si la diferencia de los tiempos establecidos es igual a deltaT */
      int respAnt = 1;
      for (int x = 0; x < numPipes; x++)
      { /* Se itera por el número de pipes creados */
        sem_wait(&semAcceso[x]);
        *memoriasCompartidasAcceso[x] = 1;
        sem_post(&semAcceso[x]);
      }
      usleep(1);
      for (int x = 0; x < numPipes; x++)
      {
        sem_wait(&semValor[x]);
        int obt = (*memoriasCompartidasValor[x]);
        printf("%d\n",obt);
        respAnt = respAnt * obt;
        sem_post(&semValor[x]);
        sem_wait(&semAcceso[x]);
        *memoriasCompartidasAcceso[x] = 0;
        sem_post(&semAcceso[x]);
      }

      if (respAnt) 
      {
        printf("Alarma encendida\n");
      }
      else
      {
        printf("Alarma no encendida\n");
      }
      time(&start_t);
    }
  }
  printf("Murio el programa\n");
  return 0;
}

bool esCoop(int tipoS)
{
  return tipoS >= 5; /* Se devuelve el valor booleano si es mayor o igual a 5 */
}

Sensor_t crearSensor(int id, int tipoS, int th, int comm)
{
  Sensor_t *sensor = (Sensor_t *)malloc(sizeof(Sensor_t)); /* Espacio de memoria del sensor */
  sensor->id = id;                                         /* Se asigna el id */
  sensor->tipoS = tipoS;                                   /* Se asigna el tipoS */
  sensor->th = th;                                         /* Se asigna el th */
  sensor->activo = 0;                                      /* Empieza el sensor siendo inactivo */
  sensor->comm = comm;                                     /* Se asigna el comm */
  return *sensor;
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

float average(Array a)
{
  float sum = 0; /* Se inicializa la suma */
  for (int i = 0; i < a.used; i++)
  {                         /* Se itera por el número de elementos existentes en el arreglo */
    sum = sum + a.array[i]; /* Se realiza la sumatoria */
  }
  return sum / (float)a.used; /* Se devuelve el promedio */
}

void cambiarEstado(Sensor_t *sensor)
{
  if (sensor->activo)
  {                     /* Si el sensor se encuentra activo */
    sensor->activo = 0; /* Se desactiva */
  }
  else
  {                     /* Si se encuentra inactivo */
    sensor->activo = 1; /* El sensor se activa */
  }
}

float S_coop(nodoSensor_t *lista)
{
  nodoSensor_t *iterador;        /* Se crea un iterador */
  iterador = lista;              /* El iterador apunta a la lista de sensores */
  int sumaNumerador = 0;         /* Se crea la variable que almacena el numerador */
  int numeroSensoresActivos = 0; /* Se crea la variable que almacena el denominador */
  while (iterador != NULL)
  {                                     /* Mientras el iterador no apunte a Null */
    Sensor_t sensor = iterador->sensor; /* Se obtiene el sensor */
    if (sensor.activo)
    {            /* Se pregunta si el sensor se encuentra activo */
      int valor; /* Se crea la variable que almacena el valor */
      for (int i = 0; i < sensor.lecturas.used; i++)
      { /* Se itera en la lista de lecturas de dicho sensor */
        if (aprobado(sensor, i))
        {            /* Si la lectura es válida */
          valor = 1; /* Valor toma el valor de 1 */
        }
        else
        {            /* Si la lectura no es válida */
          valor = 0; /* Valor toma el valor de 0 */
        }
        sumaNumerador += valor; /* Se procede a sumar el valor */
      }
      numeroSensoresActivos++; /* Se aumenta la cantidad de sensores activos */
    }
    borrarLecturasSensor(&sensor);  /* Se borran las lecturas de dicho sensor */
    iterador = iterador->siguiente; /* Se pasa al siguiente nodoSensor */
  }
  float val; /* Se crea la variable que almacena el cálculo final */
  if (numeroSensoresActivos)
  {                                              /* Si el número de sensores activos es mayor que 0 */
    val = sumaNumerador / numeroSensoresActivos; /* Se procede a calcular el valor final */
  }
  else
  {            /* Si el número de sensores activos es igual 0 */
    val = 0.0; /* El valor final es igual a 0 para evitar la división de 0 */
  }
  //printf("Sensores cooperativos %d = %.6f",numeroSensoresActivos,val); /* Se imprime por pantalla el valor final obtenido */
  return val;
}

bool aprobado(Sensor_t sensor, int i)
{
  return (sensor.lecturas.array[i] > sensor.th); /* Nos devuelve si dicha lectura es mayor al th del sensor */
}

void inicializarLecturasSensor(Sensor_t *sensor)
{
  initArray(&(sensor->lecturas), 1); /* Se inicializa el array de lecturas del sensor */
}

void borrarLecturasSensor(Sensor_t *sensor)
{
  freeArray(&(sensor->lecturas));    /* Se libera el espacio de memoria de las lecturas del sensor */
  inicializarLecturasSensor(sensor); /* Se inicializa de nuevo el espacio de memoria de las lecturas del sensor */
}

void anadirLecturaSensor(Sensor_t *sensor, int lectura)
{
  insertArray(&(sensor->lecturas), lectura); /* Se inserta la lectura en el array de dicho sensor */
}

float varianza(Sensor_t sensor)
{
  Array lecturas = sensor.lecturas;   /* Se obtiene la lista dinámica del sensor */
  float promedio = average(lecturas); /* Se obtiene el promedio de las lecturas */
  int N = lecturas.used;              /* Se crea la variable N con el número de datos obtenidos */
  float sum = 0;                      /* Se crea la variable sum */
  for (int i = 0; i < N; i++)
  {                                                     /* Se itera por el número N */
    sum = sum + pow((lecturas.array[i] - promedio), 2); /* Se procede a sumar la diferencia al cuadrado entre la lectura y el promedio */
  }
  float var; /* Se crea la variable a devolver */
  if (N <= 1)
  {            /* Si la cantidad de datos obtenidos es 1 o 0 */
    var = sum; /* Se asigna a var el valor de sum */
  }
  else
  {                             /* Si la cantidad de datos es de 2 en adelante */
    var = sum / (float)(N - 1); /* Se calcula la varianza de una muestra */
  }
  return var;
}

void *thread_function(void *nodoSen)
{ /* Función ejecutada por el hilo que recibe un nodoSensor */
  nodoSensor_t *nodoSens = (nodoSensor_t *)nodoSen;
  Sensor_t sensor = nodoSens->sensor; /* Se obtiene el sensor del nodoSensor */
  /* Leer las lecturas y guardarlas en el sensor */
  int shmid, *shm; /* Se crean las variables para la memoria compartida */
  if ((shmid = shmget(sensor.comm, SHMSZ, 0666)) < 0)
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
    if (*shm == -1)
    {
      if (sensor.activo)
      { /* Si el sensor se encuentra activo se procede a cambiar su estado */
        cambiarEstado(&sensor);
      }
    }
    else if (*shm != val)
    { /* Si el valor de la memoria compartida es un valor nuevo */

      anadirLecturaSensor(&sensor, *shm); /* Se añade la lectura al sensor */
      if (sensor.activo == 0)
      {                         /* Si se encontraba inactivo el sensor */
        cambiarEstado(&sensor); /* Se cambia el estado del sensor */
      }
      val = *shm; /* Se guarda en val el valor del puntero 'shm' */
    }
  }
}

bool nodoSensorAprobado(nodoSensor_t *nodo)
{
  Sensor_t sensor = nodo->sensor; /* Se obtiene el sensor */
  for (int i = 0; i < sensor.lecturas.used; i++)
  { /* Se itera por la cantidad de lecturas en el sensor */
    if (!aprobado(sensor, i))
    {               /* Si alguna de sus lecturas no fueron validaz en ese deltaT */
      return false; /* Se regresa false */
    }
  }
  borrarLecturasSensor(&sensor); /* Se reinicializan las lecturas del sensor */
  return true;
}

nodoSensor_t *NodoPorMenorVarianza(nodoSensor_t *sensores)
{
  nodoSensor_t *iterador;          /* Se crea un iterador */
  iterador = sensores;             /* Se asigna el iterador a la lista de sensores */
  nodoSensor_t *seleccionado;      /* Se crea la variable que almacenara el nodoSensor escogido */
  float variance = 21674627712.09; /* Se define un valor de varianza muy alto para la comparación */
  while (iterador != NULL)
  {                                        /* Mientras el iterador no apunte a Null */
    Sensor_t sensor = iterador->sensor;    /* Se obtiene el sensor */
    float varianzaTemp = varianza(sensor); /* Se obtiene la varianza de los datos de dicho sensor */
    if (varianzaTemp < variance)
    {                          /* Si la varianza obtenida es menor a la previamente asignada */
      variance = varianzaTemp; /* Se asigna a 'variance' el valor de varianza menor */
      seleccionado = iterador; /* Se asigna el nodoSensor cuyo sensor haya tenido la menor varianza */
    }
    borrarLecturasSensor(&sensor);  /* Se reinicializan las lecturas del lector */
    iterador = iterador->siguiente; /* Se pasa al siguiente nodoSensor */
  }
  return seleccionado;
}

int cantidadSensor(nodoSensor_t *sensores)
{
  nodoSensor_t *iterador; /* Se crea un iterador */
  iterador = sensores;
  int count = 0;
  while (iterador != NULL)
  {
    count++;
    iterador = iterador->siguiente;
  }
  return count;
}
