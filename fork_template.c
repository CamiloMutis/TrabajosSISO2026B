#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>
#include <string.h>

#define tam_bloque (1024 * 1024) // Tamaño de cada bloque de memoria 1Mb
#define TIEMPO_ESPERA 60 // Tiempo de espera en segundos

// Función para aumentar la memoria 
void aumentarMemoria(int n) {
    // Reserva n bloques de memoria 
    size_t total = (size_t)n * tam_bloque; 
    // RESERVAR USANDO MALLOC
    char *memoria = malloc(total); 

    if (memoria == NULL) {
        fprintf(stderr, "Error al reservar memoria\n");
        exit(EXIT_FAILURE);
    }
    //COMPLETAR: ESCRIBIR EN MEMORIA USANDO MEMSET
    memset(memoria, 'D', total);
    printf("* [PID %d] Memoria aumentada exitosamente: %d MB\n",getpid(),n);
    
}

int main() {
    // Variable para almacenar la cantidad de subprocesos a crear
    int num_subprocesos;


    printf("PID del proceso padre: %d \n", getpid());
    printf("Ingrese el número de subprocesos a crear: ");
    fflush(stdout); //limpiar consola :D
    scanf("%d", &num_subprocesos);

    // Bucle para crear el número especificado de subprocesos
    for (int i = 0; i < num_subprocesos; i++) {
        // COMPLETAR: Crear Subproceso
        pid_t pid = fork(); //FORK

        if (pid < 0) {
            // Error al crear el subproceso hijo
            fprintf(stderr, "Error al crear el subproceso hijo\n");
            exit(EXIT_FAILURE);
        } else if (pid == 0) {
            // Proceso hijo
            printf("[Hijo %d] PID = %d, PPID = %d", i+1,getpid(),getppid());
            int n;
            scanf("%d, n");
            // Completar: pedir un número de bloques de memoria al usuario y reservar
            //llamando a la función implementada más arriba
            aumentarMemoria(n);
            //COMPLETAR:
            exit(EXIT_SUCCESS);

        }
    }

    // Esperamos a que todos los subprocesos hijos terminen
    for (int i = 0; i < num_subprocesos; i++) {
        wait(NULL);
    }

    return EXIT_SUCCESS;
}

//crear 10 sub procesos 
// ver comportamiento de los procesos en la consola de linux y jerarquia de proceso
// ps -ef
//ver arbol de procesos"
//pstree

