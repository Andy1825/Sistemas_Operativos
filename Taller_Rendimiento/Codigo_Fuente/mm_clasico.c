/**************************************************************
		Pontificia Universidad Javeriana
	Autor: J. Corredor
	Fecha: Febrero 2024
	Modificado y documentado por: A. Parrado
	Fecha modificación: Mayo 2024
	Materia: Sistemas Operativos
	Tema: Taller de Evaluación de Rendimiento
	Fichero: fuente de multiplicación de matrices NxN por hilos.
	Objetivo: Evaluar el tiempo de ejecución del 
					algoritmo clásico de multiplicación de matrices.
				Se implementa con la Biblioteca POSIX Pthreads
****************************************************************/

#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>

#define DATA_SIZE (1024*1024*64*3) 

pthread_mutex_t MM_mutex; // Mutex para sincronización de acceso a recursos compartidos
static double MEM_CHUNK[DATA_SIZE]; // Memoria compartida para almacenar las matrices
double *mA, *mB, *mC; // Punteros a las matrices A, B y C

struct parametros{
	int nH; // Número total de hilos
	int idH; // Identificador del hilo actual
	int N; // Tamaño de las matrices (NxN)
};

struct timeval start, stop; // Variables para medir el tiempo de ejecución

// Función para llenar las matrices con valores aleatorios
void llenar_matriz(int SZ){ 
	srand48(time(NULL)); // Inicialización de la semilla aleatoria
	for(int i = 0; i < SZ*SZ; i++){
		mA[i] = 1.1*i; // Inicialización de la matriz A
		mB[i] = 2.2*i; // Inicialización de la matriz B
		mC[i] = 0; // Inicialización de la matriz C
	}	
}

// Función para imprimir una matriz
void print_matrix(int sz, double *matriz){
	if(sz < 12){
		for(int i = 0; i < sz*sz; i++){
			if(i%sz==0) printf("\n");
			printf(" %.3f ", matriz[i]);
		}	
		printf("\n>-------------------->\n");
	}
}

// Funciones para medir el tiempo de ejecución
void inicial_tiempo(){
	gettimeofday(&start, NULL); // Inicio del contador de tiempo
}

void final_tiempo(){
	gettimeofday(&stop, NULL); // Fin del contador de tiempo
	stop.tv_sec -= start.tv_sec; // Cálculo del tiempo transcurrido en segundos
	printf("\n:-> %9.0f µs\n", (double) (stop.tv_sec*1000000 + stop.tv_usec)); // Impresión del tiempo transcurrido
}

// Función ejecutada por cada hilo para multiplicar por bloques de la matriz
void *mult_thread(void *variables){
	struct parametros *data = (struct parametros *)variables; // Conversión de los argumentos

	int idH = data->idH; // Identificador del hilo
	int nH  = data->nH; // Número total de hilos
	int N   = data->N; // Tamaño de las matrices
	int ini = (N/nH)*idH; // Índice inicial para la división de trabajo
	int fin = (N/nH)*(idH+1); // Índice final para la división de trabajo

	for (int i = ini; i < fin; i++){ // Bucle sobre las filas del bloque asignado al hilo
		for (int j = 0; j < N; j++){ // Bucle sobre las columnas de la matriz B
			double *pA, *pB, sumaTemp = 0.0; // Punteros y variable temporal para la suma
			pA = mA + (i*N); // Puntero al inicio de la fila i de la matriz A
			pB = mB + j; // Puntero a la columna j de la matriz B
			for (int k = 0; k < N; k++, pA++, pB+=N){ // Bucle para multiplicar y sumar los elementos
				sumaTemp += (*pA * *pB); // Multiplicación y acumulación de productos
			}
			mC[i*N+j] = sumaTemp; // Asignación del resultado a la posición correspondiente de la matriz C
		}
	}

	pthread_mutex_lock (&MM_mutex); // Bloqueo del mutex para sincronización
	pthread_mutex_unlock (&MM_mutex); // Desbloqueo del mutex
	pthread_exit(NULL); // Salida del hilo
}

int main(int argc, char *argv[]){
	if (argc < 2){
		printf("Ingreso de argumentos \n $./ejecutable tamMatriz numHilos\n"); // Verificación de argumentos de línea de comandos
		return -1;	
	}
	int SZ = atoi(argv[1]); // Tamaño de las matrices
	int n_threads = atoi(argv[2]); // Número de hilos a utilizar

	pthread_t p[n_threads]; // Arreglo de identificadores de hilos
	pthread_attr_t atrMM; // Atributos para los hilos

	mA = MEM_CHUNK; // Asignación de memoria para la matriz A
	mB = mA + SZ*SZ; // Asignación de memoria para la matriz B
	mC = mB + SZ*SZ; // Asignación de memoria para la matriz C

	llenar_matriz(SZ); // Llenado de las matrices con valores aleatorios
	print_matrix(SZ, mA); // Impresión de la matriz A
	print_matrix(SZ, mB); // Impresión de la matriz B

	inicial_tiempo(); // Inicio de la medición del tiempo
	pthread_mutex_init(&MM_mutex, NULL); // Inicialización del mutex
	pthread_attr_init(&atrMM); // Inicialización de los atributos de los hilos
	pthread_attr_setdetachstate(&atrMM, PTHREAD_CREATE_JOINABLE); // Establecimiento de los atributos para permitir la unión de los hilos

	for (int j=0; j<n_threads; j++){ // Bucle para la creación de los hilos
		struct parametros *datos = (struct parametros *) malloc(sizeof(struct parametros)); // Asignación de memoria para los argumentos del hilo
		datos->idH = j; // Asignación del identificador del hilo
		datos->nH  = n_threads; // Asignación del número total de hilos
		datos->N   = SZ; // Asignación del tamaño de las matrices
		pthread_create(&p[j],&atrMM,mult_thread,(void *)datos); // Creación del hilo con los argumentos dados
	}

	for (int j=0; j<n_threads; j++) // Bucle para la unión de los hilos
		pthread_join(p[j],NULL); // Unión de los hilos con el hilo principal

	final_tiempo(); // Finalización de la medición del tiempo

	print_matrix(SZ, mC); // Impresión de la matriz resultante C

	pthread_attr_destroy(&atrMM); // Destrucción de los atributos de los hilos
	pthread_mutex_destroy(&MM_mutex); // Destrucción del mutex
	pthread_exit (NULL); // Salida del programa
}
