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

pthread_mutex_t MM_mutex; // Mutex para sincronización de hilos
static double MEM_CHUNK[DATA_SIZE]; // Chunk de memoria estática para las matrices
double *mA, *mB, *mC; // Punteros para las matrices

struct parametros{
	int nH; // Número total de hilos
	int idH; // ID del hilo actual
	int N;   // Tamaño de las matrices
};

struct timeval start, stop; // Variables para medir el tiempo

// Función para llenar una matriz con valores aleatorios
void llenar_matriz(int SZ){ 
	srand48(time(NULL));
	for(int i = 0; i < SZ*SZ; i++){
			mA[i] = 1.1*i; // Se llenan las matrices con valores determinados (puede ser randómico)
			mB[i] = 2.2*i; // Aquí se puede usar drand48() para valores aleatorios
			mC[i] = 0;     // Inicialización de la matriz resultante en cero
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
	gettimeofday(&start, NULL);
}

void final_tiempo(){
	gettimeofday(&stop, NULL);
	stop.tv_sec -= start.tv_sec;
	printf("\n:-> %9.0f µs\n", (double) (stop.tv_sec*1000000 + stop.tv_usec));
}

// Función que realiza la multiplicación de matrices en un hilo
void *mult_thread(void *variables){
	struct parametros *data = (struct parametros *)variables;

	int idH = data->idH;
	int nH  = data->nH;
	int N   = data->N;
	int ini = (N/nH)*idH; // Inicio del rango de filas que el hilo debe multiplicar
	int fin = (N/nH)*(idH+1); // Fin del rango de filas

		for (int i = ini; i < fin; i++){
				for (int j = 0; j < N; j++){
			double *pA, *pB, sumaTemp = 0.0;
			pA = mA + (i*N); 
			pB = mB + (j*N);
						for (int k = 0; k < N; k++, pA++, pB++){
				sumaTemp += (*pA * *pB); // Multiplicación de matrices convencional
			}
			mC[i*N+j] = sumaTemp; // Almacenamiento del resultado en la matriz de resultado
		}
	}

	pthread_mutex_lock (&MM_mutex); // Bloqueo del mutex
	pthread_mutex_unlock (&MM_mutex); // Desbloqueo del mutex
	pthread_exit(NULL); // Finalización del hilo
}

// Función principal
int main(int argc, char *argv[]){
	if (argc < 2){
		printf("Ingreso de argumentos \n $./ejecutable tamMatriz numHilos\n");
		return -1;	
	}
		int SZ = atoi(argv[1]); // Tamaño de la matriz NxN
		int n_threads = atoi(argv[2]); // Número de hilos a utilizar

		pthread_t p[n_threads]; // Arreglo de hilos
		pthread_attr_t atrMM; // Atributos de los hilos

	mA = MEM_CHUNK; // Asignación de memoria para la matriz A
	mB = mA + SZ*SZ; // Asignación de memoria para la matriz B
	mC = mB + SZ*SZ; // Asignación de memoria para la matriz C

	llenar_matriz(SZ); // Llenar las matrices con valores

	print_matrix(SZ, mA); // Imprimir la matriz A
	print_matrix(SZ, mB); // Imprimir la matriz B

	inicial_tiempo(); // Iniciar la medición del tiempo
	pthread_mutex_init(&MM_mutex, NULL); // Inicializar el mutex
	pthread_attr_init(&atrMM); // Inicializar los atributos de los hilos
	pthread_attr_setdetachstate(&atrMM, PTHREAD_CREATE_JOINABLE); // Configurar los hilos como joinable

		for (int j=0; j<n_threads; j++){
		struct parametros *datos = (struct parametros *) malloc(sizeof(struct parametros)); // Reserva de memoria para los parámetros de los hilos
		datos->idH = j; // ID del hilo
		datos->nH  = n_threads; // Número total de hilos
		datos->N   = SZ; // Tamaño de las matrices
				pthread_create(&p[j],&atrMM,mult_thread,(void *)datos); // Crear el hilo con los parámetros correspondientes
	}

		for (int j=0; j<n_threads; j++)
				pthread_join(p[j],NULL); // Esperar a que todos los hilos terminen su ejecución
	final_tiempo(); // Finalizar la medición del tiempo

	print_matrix(SZ, mC); // Imprimir la matriz resultante

	pthread_attr_destroy(&atrMM); // Destruir los atributos de los hilos
	pthread_mutex_destroy(&MM_mutex); // Destruir el mutex
	pthread_exit (NULL); // Salir del programa
}

