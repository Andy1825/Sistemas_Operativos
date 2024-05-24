/**************************************************
IMPLEMENTACIÓN PROCESO SENSOR - PRODUCTOR
Autor: Andres Parrado
Fecha Creación: 29 Abril 2024
Fecha Actualización: 22 Mayo 2024
Materia: Sistemas Operativos
Pontificia Universidad Javeriana
Proyecto Final: Sensores y Monitor
***************************************************/

#include <fcntl.h>  //Librería para manipulación de archivos
#include <stdio.h>  //Librería para funciones de entrada y salida
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

// Función main del programa Sensor
int main(int argc, char *argv[]) {

  /*Validación del ingreso de datos: Teniendo en cuenta que la estructura del
  ejecutable es "./sensor -s tipo_sensor -t tiempo -f archivo -p pipe_nominal"
  si el número de argumentos es distinto a 9 (argc), se arrojara una advertencia
  al usuario de seguir la estructura que entiende el programa y este mismo se
  cerrará.
  */
  if (argc != 9) {
    printf("Estructura invalida, verifique que siga el siguiente patrón: %s -s "
           "tipo_sensor -t tiempo -f archivo -p pipe_nominal\n",
           argv[0]);    // Nombre del ejecutable del programa (%s)
    exit(EXIT_FAILURE); // Termina ejecución del programa
  }

  // Declaración de las variables que el usuario digita en la shell
  int tipo_sensor = 0, tiempo = 0;
  char *archivo = NULL, *pipe_nominal = NULL;

  /*Parseo de los argumentos de la línea de comandos: Mediante la función
  strcmp, se realiza la comparación de las banderas de cada tipo de dato, si la
  cadena comparada coincide con el argumento, la función devuelve 0 y se
  almacena el dato correspondiente a dicha bandera. El bucle comienza desde el
  segundo argumento (-s) y avanza de dos en dos para determinar cada bandera. El
  tipo de sensor y el tiempo al ser cadenas se le realiza un casteo mediante la
  función atoi que los convierte en números.
  */
  for (int i = 1; i < argc; i += 2) {
    if (strcmp(argv[i], "-s") == 0)
      tipo_sensor = atoi(argv[i + 1]);
    else if (strcmp(argv[i], "-t") == 0)
      tiempo = atoi(argv[i + 1]);
    else if (strcmp(argv[i], "-f") == 0)
      archivo = argv[i + 1];
    else if (strcmp(argv[i], "-p") == 0)
      pipe_nominal = argv[i + 1];
  }

  /*Validación de argumentos: Se valida que los datos ingresados sean distintos
  a 0 o nulos, en el caso del sensor si este es distinto a 1 o 2 el programa
  termina su ejecución */
  if ((tipo_sensor != 1 && tipo_sensor != 2) || tiempo == 0 ||
      archivo == NULL || pipe_nominal == NULL) {
    printf("Faltan argumentos de entrada!\n");
    exit(EXIT_FAILURE);
  }

  /*Creación del Pipe Nominal: Antes de realizar la creación del pipe se valida
  si este existe, mediante la función access y el argumneto F_OK que devuelve -1
  si no existe, si no existe se crea el pipe de tipo FIFO con el nombre
  especificado por el usuario, utilizando la función makefifo y el argumento
  0666 que otorga permisos de lectura y escritura. Si la creación del pipe
  falla, se arroja un valor de -1 y se cierra el programa.
  */
  if (access(pipe_nominal, F_OK) == -1) {
    if (mkfifo(pipe_nominal, 0666) == -1) {
      perror("Error al crear el pipe nominal");
      exit(EXIT_FAILURE);
    }
  }

  /*Apertura del pipe nominal: Se realiza la abertura del pipe nominal mediante
  la función open y el argumento O_WRONLY que indica que se abrira unicamente en
  modo escritura. Si el descriptor de archivo "pipe" es menor a 0 quiere decir
  que la abertura fallo y se cierra el programa.
  */
  int pipe = open(pipe_nominal, O_WRONLY);
  if (pipe < 0) {
    perror("Error al abrir el pipe nominal para escritura");
    exit(EXIT_FAILURE);
  }

  /*Apertura del archivo: Se realiza la apertura de los datos del archivo con
  las medidas del sensor, mediante la función fopen y el argumento "r" que abren
  el archivo en modo lectura. Este a su vez devuelve un puntero a la
  estructura FILE, que representa el archivo abierto, si este es nulo quiere
  decir que ocurrio un error al abrir el archivo y el programa termina su
  ejecución.
  */
  FILE *file = fopen(archivo, "r");
  if (file == NULL) {
    perror("Error al abrir el archivo");
    exit(EXIT_FAILURE);
  }

  // Se realiza la declaración del buffer donde se almacenaran los datos
  char buffer[BUF_SIZE];

  // Declaración de la variable medida, que almacenara las medidas leidas en el
  // archivo de texto
  float medida;

  printf("Iniciando sensores...\n");

  sleep(2);

  /*Lectura del archivo y escritura de datos en el pipe: Mediente la función
  fgets se realiza la lectura de los datos del archivo, mientras no alcance el
  final del mismo (EOF). Se leen los datos de las lineas de hasta el tamaño
  maximo de caracteres (BUF_SIZE) y se almacena en el buffer creado, si no hay
  mas lineas por leer la funcion retorna nulo y la lectura finaliza.

  Seguidamente se valida si la medida es mayor o igual a 0, sino esta no sera
  escrita en el pipe, si por el contrario si cumple la condición se realiza la
  escritura de los datos en el pipe.
  */
  while (fgets(buffer, BUF_SIZE, file) != NULL) {
    medida = atof(buffer); // Conversion caracter a dato flotante
    printf("\n----------------------------------------------------\n");
    if (medida >= 0) {
      if ((tipo_sensor == 1 && (medida >= 20 && medida <= 31.6)) ||
          (tipo_sensor == 2 && (medida >= 6.0 && medida <= 8.0))) {
        // Si la medida está dentro del rango
        printf("\nTipo %d ---> Lectura: %.2f -> Dentro del rango\n",
               tipo_sensor, medida); // Mensaje de depuración
      } else {
        // Si la medida está fuera del rango
        printf("\nTipo %d ---> Lectura: %.2f -> Fuera del rango\n", tipo_sensor,
               medida); // Mensaje de depuración
      }

      /* Creación paquete de datos que se enviaran al pipe. Este contiene la
      medida y el tipo de sensor de esta medida*/
      struct SensorData data;
      data.tipo_sensor = tipo_sensor;
      data.medida = medida;

      // Mensaje de depuración: Impresión de lo que se envia al pipe
      printf("Enviando al pipe: Tipo %d - Medida: %.2f\n", data.tipo_sensor,
             data.medida);

      /*Escritura de datos en el pipe: Dentro del bucle de la lectura del
      archivo se realiza la escritura de los datos validos en el pipe. Esto se
      raliza mediante la función write que recibe el pipe, la dirección de
      memoria de la estructura de datos y el tamaño de la misma. La función
      devuelve el número de bytes escritos, si retorna un numero menor a 0
      quiere decir que la escritura fallo y se cierra el programa.
      */
      ssize_t bytes_written = write(pipe, &data, sizeof(struct SensorData));
      if (bytes_written < 0) {
        perror("Error escritura en el pipe");
        exit(EXIT_FAILURE);
      }
    }

    else {
      printf("\nNo se envia al monitor: %.2f -> Es negativo\n", medida);
    }

    sleep(tiempo); // Espera tiempo especificado entre medidas
  }

  fclose(file); // Se cierra el archivo leido
  close(pipe);  // Se cierra el lado de la pipe del sensor

  return 0;
}
