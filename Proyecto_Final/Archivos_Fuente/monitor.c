/**************************************************
IMPLEMENTACIÓN PROCESO MONITOR - CONSUMIDOR
Autor: Andres Parrado
Fecha Creación: 29 Abril 2024
Fecha Actualización: 22 Mayo 2024
Materia: Sistemas Operativos
Pontificia Universidad Javeriana
Proyecto Final: Sensores y Monitor
***************************************************/

#include <fcntl.h>     //Librería para manipulación de archivos
#include <pthread.h>   //Librería para gestión y sincronización de hilos
#include <semaphore.h> //Librería para el manejo de semaforos
#include <stdio.h>     //Librería para funciones de entrada y salida
#include <stdlib.h> //Librería para casteos y asignación de memoria dinámica
#include <string.h> //Librería para manipulación de strings
#include <sys/stat.h> //Librería para conocer estados de los archivos
#include <time.h>     //Librería para manipulación del tiempo
#include <unistd.h>   //Librería para lectura y escritura de pipes

// Definición tamaño del buffer
#define BUF_SIZE 100

/*Estructura para almacenar los datos del sensor que seran pasados al pipe (Tipo
sensor y medida)*/
struct SensorData {
  int tipo_sensor; // Tipo de sensor (1.Temperatura - 2.PH)
  float medida;    // Medida detectada por el sensor
};

/*Creación de la estructura del Buffer donde se almacenarán las medidas y las
caracteristicas del mismo y los semáforos*/
struct Buffer {
  struct SensorData data[BUF_SIZE];
  int inicio;      // Indice del primer elemento del buffer
  int fin;         // Indice del proximo espacio disponible del buffer
  int contador;    // Indice del búmero de elementos en el buffer
  sem_t sem_mutex; // Semaforo para controlar elacceso al buffer
  sem_t sem_items; // Semaforo que indica la disponibilidad de elementos del
                   // buffer
};

// Creación de los buffer de temperatura y ph de tipo Buffer
struct Buffer buffer_temperatura, buffer_ph;

/*Creación de la función recolector: Para recibir las medidas del pipe y
almacenarlas en el buffer correspondiente se crea la función recolector que
recibe un parametro void  y devuelve un puntero, que sera el que necesitara para
la creación del hilo. Durante la función se abrira el pipe, se recibieran las
medidas y el tipo de sensor para dicha medida, y despues se clasificaran en el
buffer correspondiente*/
void *recolector(void *arg) {
  int pipe_fd =
      *((int *)arg); // Declaración del descriptor de archivo del pipe nominal
  struct SensorData data; // Declaración estructura de los datos del sensor

  /*Creación bucle infinito hasta cumplir con la condición de que todos los
  datos del pipe fueron leidos, esto es hasta cuando el numero de bytes es igual
  a 0, esto quiere decir que todos los datos enviados por el sensor fueron
  leidos*/
  while (1) {
    /*Se realiza la lectura de de los datos del pipe nominal, en este caso es la
    estructura de los datos de la medida del sensor. Si la función de lectura
    devuelve un numero menor a 1 quere decir que la lectura del pipe fallo y el
    programa termina*/
    ssize_t bytes_read = read(pipe_fd, &data, sizeof(struct SensorData));
    if (bytes_read < 0) {
      perror("Error al leer del pipe");
      exit(EXIT_FAILURE);
    } else if (bytes_read == 0) {
      break;
    }
    /*Se realiza la gestión de los semaforos para la sincronización de los
    hilos, primero mediante la función sem_wait se hace esperar a un hilo hata
    que el semaforo de exclusion mutua este disponible con el fin de evitar
    condiciones de carrera*/
    sem_wait(&buffer_temperatura.sem_mutex);

    /*Si el tipo de sensor recibido 1 y no se supera el tamaño del buffer se escribe la
    medida en la ultima posición disponible del buffer, a medida que trancurre el bucle el
    indice de fin se actualiza corriendo una posición en el buffer, el residuo con el tamaño
    del buffer impide que la ultima posición sobrepase el tamaño del buffer, a su vez el 
    contador de elementos aumenta en 1*/
    if (data.tipo_sensor == 1 && buffer_temperatura.contador < BUF_SIZE) {
      buffer_temperatura.data[buffer_temperatura.fin] = data;
      buffer_temperatura.fin = (buffer_temperatura.fin + 1) % BUF_SIZE;
      buffer_temperatura.contador++;

      /*Mediante la función sem_post se incrementa el semaforo de items, indicando que hay un 
      elemneto adicional en el buffer, permitiendo que los hilos de PH o Temperatura lean del
      buffer, si no se detecta una nueva medida, los hilos no pueden leer*/
      sem_post(&buffer_temperatura.sem_items);
      
      /*Si el tipo de sensor recibido 1 y no se supera el tamaño del buffer se escribe la
      medida en la ultima posición disponible del buffer, a medida que trancurre el bucle el
      indice de fin se actualiza corriendo una posición en el buffer, el residuo con el tamaño
      del buffer impide que la ultima posición sobrepase el tamaño del buffer, a su vez el 
      contador de elementos aumenta en 1*/
    } else if (data.tipo_sensor == 2 && buffer_ph.contador < BUF_SIZE) {
      buffer_ph.data[buffer_ph.fin] = data;
      buffer_ph.fin = (buffer_ph.fin + 1) % BUF_SIZE;
      buffer_ph.contador++;
      
      sem_post(&buffer_ph.sem_items);
    } else {
      printf("Error: Buffer lleno, descartando medida.\n");
    }
    //Indicación de que se completo una operación crítica y que algún otro hilo puede acceder al buffer
    sem_post(&buffer_temperatura.sem_mutex);
  }

  printf("Esperando medidas de algún otro sensor ...\n");
  sleep(10); // Esperar 10 segundos si no hay más datos

  // 
  sem_wait(&buffer_temperatura.sem_mutex);

  //Se marcan el tipo de sensor y medida en valores especiales para indicar que termino
  buffer_temperatura.data[buffer_temperatura.fin].tipo_sensor = -1;
  buffer_temperatura.data[buffer_temperatura.fin].medida = 0;
  buffer_ph.data[buffer_ph.fin].tipo_sensor = -1;
  buffer_ph.data[buffer_ph.fin].medida = 0;
  
  sem_post(&buffer_temperatura.sem_items);
  sem_post(&buffer_ph.sem_items);
  sem_post(&buffer_temperatura.sem_mutex);

  // Cierre del pipe
  close(pipe_fd);

  printf("Procesamiento de medidas finalizado.\n");

  pthread_exit(NULL);
}

void *procesar(void *arg) {

  //Casteo de puntero void a uno de estructura Buffer para pasar los datos en el pthread_create
  struct Buffer *buffer = (struct Buffer *)arg;
  
  FILE *file;

  //En esta version preliminar los nombres de los archivos por ahora es constante
  char *filename = buffer == &buffer_temperatura ? "file-temp.txt" : "file-ph.txt";

  /*Apertura del archivo: Se realiza la apertura de los datos del archivo donde 
  se pondran las medidas, mediante la función fopen y el argumento "w" que abren
  el archivo en modo escritura. Este a su vez devuelve un puntero a la
  estructura FILE, que representa el archivo abierto, si este es nulo quiere
  decir que ocurrio un error al abrir el archivo y el programa termina su
  ejecución.
  */
  file = fopen(filename, "w");
  if (file == NULL) {
    perror("Error al abrir el archivo");
    exit(EXIT_FAILURE);
  }

  //Bucle infinito mientras que no se lean todos las medidas del pipe
  while (1) {
    //Se espera hasta que haya un elemento en el buffer
    sem_wait(&buffer->sem_items);

    //Evita que algun hilo modifique el buffer mientras que otro esta leyendo las medidas
    sem_wait(&buffer->sem_mutex);

    //Si la posición inicial es igual a el tamaño del buffer se reinicia al principio
    struct SensorData data = buffer->data[buffer->inicio];
    buffer->inicio = (buffer->inicio + 1) % BUF_SIZE;
    buffer->contador--;
    
    //Liberación semaforo mutex para que otro hilo pueda leer el buffer
    sem_post(&buffer->sem_mutex);

    /*Cuando se cumple la condición especial del tipo de sensor en la función recolector 
    los archivos se cierran*/
    if (data.tipo_sensor == -1) {
      fclose(file);
      printf("Archivo %s cerrado.\n", filename);
      pthread_exit(NULL);
    }

    //Declaración variables para encontrar la hora actual
    char buffer_time[26];
    time_t rawtime;
    struct tm *timeinfo;

    /* Escritura de la hora actual en la que se escribe la medida en el archivo nuevo, 
    mediente la función strftime*/
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(buffer_time, sizeof(buffer_time), "%H:%M:%S", timeinfo);

    /* Verificar si la medida está dentro de los rangos permitidos, dependiendo si
    es de PH o temperatura*/
    int dentro_de_rango = 0;
    if ((data.tipo_sensor == 1 && (data.medida >= 20 && data.medida <= 31.6)) ||
        (data.tipo_sensor == 2 && (data.medida >= 6.0 && data.medida <= 8.0))) {
      dentro_de_rango = 1;
    }

    // Escritura de las medidas en el archivo
    fprintf(file, "%s --> %.2f\n", buffer_time, data.medida);
    if (!dentro_de_rango) {
      //Si esta fera del rango imprime un mensaje por pantalla
      printf("Alerta: Medida fuera del rango en %s.\n", filename);
    }
  }
}

int main(int argc, char *argv[]) {

  /*Validación del ingreso de datos: Teniendo en cuenta que la estructura del 
  ejecutable es "./monitor -b tam_buffer -t file-temp -h file-ph -p pipe-nominal" 
  si el número de argumentos es distinto a 9 (argc), se arrojara una advertencia al 
  usuario de seguir la estructura que entiende el programa y este mismo se cerrará.
  */
  if (argc != 9) {
    fprintf(stderr,
            "Uso: %s -b tam_buffer -t file-temp -h file-ph -p pipe-nominal\n",
            argv[0]); // Nombre del ejecutable del programa (%s)
    exit(EXIT_FAILURE); // Termina ejecución del programa
  }

  //Declaración de las variables que el usuario digita en la shell
  int tam_buffer;
  char *file_temp, *file_ph, *pipe_nominal;

   /*Parseo de los argumentos de la línea de comandos: Mediante la función strcmp,
   se realiza la comparación de las banderas de cada tipo de dato, si la cadena 
   comparada coincide con el argumento, la función devuelve 0 y se almacena el dato 
   correspondiente a dicha bandera. El bucle comienza desde el segundo argumento (-b)
   y avanza de dos en dos para determinar cada bandera.
   El tamaño del buffer al ser cadena se le realiza un casteo mediante la función atoi 
   que lo convierte a número entero.
   */
  for (int i = 1; i < argc; i += 2) {
    if (strcmp(argv[i], "-b") == 0)
      tam_buffer = atoi(argv[i + 1]);
    else if (strcmp(argv[i], "-t") == 0)
      file_temp = argv[i + 1];
    else if (strcmp(argv[i], "-h") == 0)
      file_temp = argv[i + 1];
    else if (strcmp(argv[i], "-p") == 0)
      pipe_nominal = argv[i + 1];
  }

  // Inicializar buffer de temperatura
  buffer_temperatura.inicio = 0;
  buffer_temperatura.fin = 0;
  buffer_temperatura.contador = 0;

  /*Mediante la función sem_init se inician los semaforos de mutex que al ser 1 implica 
  que el semaforo logrará exclusion mutua, y es 0 para los itemas del buffer indicando que
  en un inicio el buffer de temperatura esta vacio*/
  sem_init(&buffer_temperatura.sem_mutex, 0, 1);
  sem_init(&buffer_temperatura.sem_items, 0, 0);
  
  // Inicializar buffer de PH
  buffer_ph.inicio = 0;
  buffer_ph.fin = 0;
  buffer_ph.contador = 0;

  /*Mediante la función sem_init se inician los semaforos de mutex que al ser 1 implica 
  que el semaforo logrará exclusion mutua, y es 0 para los iteas del buffer indicando que
  en un inicio el buffer de PH esta vacio*/
  sem_init(&buffer_ph.sem_mutex, 0, 1);
  sem_init(&buffer_ph.sem_items, 0, 0);

  // Creación del hilo recolector de medidas del pipe:
  pthread_t hilo_recolector;

  /*Apertura del pipe nominal: Se realiza la abertura del pipe nominal mediante
  la función open y el argumento O_RDONLY que indica que se abrira unicamente en
  modo lectura. Si el descriptor de archivo "pipe" es menor a 0 quiere decir
  que la abertura fallo y se cierra el programa.*/
  int pipe_fd = open(pipe_nominal, O_RDONLY);
  if (pipe_fd < 0) {
    perror("Error al abrir el pipe nominal para lectura");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&hilo_recolector, NULL, recolector, &pipe_fd) != 0) {
    perror("Error al crear el hilo recolector");
    exit(EXIT_FAILURE);
  }

  /* Creación de hilos para la escritura del PH y la temperatura, si la función
  de creación devuelve un numero distinto a 0 la creación del hilo fallo y el
  programa termina*/
  pthread_t hilo_ph, hilo_temp;
  if (pthread_create(&hilo_ph, NULL, procesar, &buffer_ph) != 0) {
    perror("Error al crear el hilo H-ph");
    exit(EXIT_FAILURE);
  }
  if (pthread_create(&hilo_temp, NULL, procesar, &buffer_temperatura) != 0) {
    perror("Error al crear el hilo H-temperatura");
    exit(EXIT_FAILURE);
  }

  // Esperar a que los hilos terminen
  pthread_join(hilo_recolector, NULL);
  pthread_join(hilo_ph, NULL);
  pthread_join(hilo_temp, NULL);

  // Destrucción de los semaforos por medio de la función sem_destroy
  sem_destroy(&buffer_temperatura.sem_mutex);
  sem_destroy(&buffer_temperatura.sem_items);
  sem_destroy(&buffer_ph.sem_mutex);
  sem_destroy(&buffer_ph.sem_items);

  return 0;
}
