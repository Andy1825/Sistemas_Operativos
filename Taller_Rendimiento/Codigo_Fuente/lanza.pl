#!/usr/bin/perl
#**************************************************************
#         		Pontificia Universidad Javeriana
#     Autor: J. Corredor
#     Fecha: Febrero 2024
#     Materia: Sistemas Operativos
#     Tema: Taller de Evaluación de Rendimiento
#     Fichero: script automatización ejecución por lotes 
#****************************************************************/

# Configuración inicial del script
$Path = `pwd`; # Obtiene el directorio actual
chomp($Path); # Elimina el salto de línea del directorio

# Definición de variables
$Nombre_Ejecutable = "MM_clasico"; # Nombre del ejecutable a utilizar(MM_clasico o MM_transpuesto)
@Size_Matriz =("50"); # Tamaños de las matrices a procesar
@Num_Hilos = (2,4,8); # Números de hilos a utilizar
$Repeticiones = 10; # Número de repeticiones por configuración

# Bucle para recorrer los tamaños de las matrices
foreach $size (@Size_Matriz){
    # Bucle para recorrer los números de hilos
	foreach $hilo (@Num_Hilos) {
        # Creación del nombre del archivo de salida
		$file = "$Path/$Nombre_Ejecutable-".$size."-Hilos-".$hilo.".dat";
        # Bucle para realizar las repeticiones
		for ($i=0; $i<$Repeticiones; $i++) {
            # Ejecución del comando para correr el ejecutable y redirigir la salida al archivo
			system("$Path/$Nombre_Ejecutable $size $hilo  >> $file");
            # Impresión del comando que se ejecutaría (comentado)
			 # printf("$Path/$Nombre_Ejecutable $size $hilo \n");
		}
		close($file); # Cierra el archivo (aunque no es necesario porque no se abrió explícitamente)
		$p=$p+1; # Incrementa un contador (aunque no se utiliza posteriormente)
	}
}