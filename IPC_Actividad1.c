#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define FIFO_SUMA "fifo_suma"
#define FIFO_MUL "fifo_mul"
#define BUFFER_SIZE 128
#define MAX 10

struct Datos {
    int a[MAX];
    int b[MAX];
    int resultado_sum[MAX];
    int resultado_mul[MAX];
    int n_sum;
    int n_mul;
};

int main(void) {
    int shm_id;
    struct Datos *shmatch;

    shm_id = shmget(IPC_PRIVATE, sizeof(struct Datos), IPC_CREAT | 0600);
    if (shm_id < 0) {
        perror("Error creando memoria compartida");
        exit(1);
    }

    shmatch = shmat(shm_id, NULL, 0);
    if (shmatch == (struct Datos *) -1) {
        perror("Error asociando memoria compartida");
        exit(1);
    }

    shmatch->n_sum = 0;
    shmatch->n_mul = 0;

    mkfifo(FIFO_SUMA, 0666);
    mkfifo(FIFO_MUL, 0666);

    printf("Padre: PID %d\n", getpid());
    fflush(stdout);

    // Productor
    if (fork() == 0) {
        int fd_suma = open(FIFO_SUMA, O_WRONLY);
        int fd_mul = open(FIFO_MUL, O_WRONLY);
        if (fd_suma < 0 || fd_mul < 0) {
            perror("Error abriendo FIFO para escritura");
            exit(1);
        }

        for (int i = 1; i <= 5; i++) {
            char buffer[BUFFER_SIZE];
            memset(buffer, 0, BUFFER_SIZE);
            snprintf(buffer, BUFFER_SIZE, "%d %d", i, i + 2);
            write(fd_suma, buffer, BUFFER_SIZE);
            write(fd_mul, buffer, BUFFER_SIZE);
            printf("Productor: enviado %s\n", buffer);
            fflush(stdout);
            sleep(1);
        }

        close(fd_suma);
        close(fd_mul);
        shmdt(shmatch);
        exit(0);
    }

    // Hijo 1: suma
    if (fork() == 0) {
        int fd = open(FIFO_SUMA, O_RDONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO de suma");
            exit(1);
        }

        int *resultados = malloc(MAX * sizeof(int));
        int cantidad = 0;
        char buffer[BUFFER_SIZE];
        int a, b;

        while (read(fd, buffer, BUFFER_SIZE) > 0 && cantidad < MAX) {
            sscanf(buffer, "%d %d", &a, &b);
            int resultado_sum = a + b;
            resultados[cantidad] = resultado_sum;
            cantidad++;

            shmatch->resultado_sum[shmatch->n_sum] = resultado_sum;
            shmatch->n_sum++;

            printf("Hijo 1 Suma: PID %d, %d + %d = %d\n", getpid(), a, b, resultado_sum);
            fflush(stdout);
        }

        free(resultados);
        close(fd);
        shmdt(shmatch);
        exit(0);
    }

    // Hijo 2: multiplicacion
    if (fork() == 0) {
        int fd = open(FIFO_MUL, O_RDONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO de multiplicacion");
            exit(1);
        }

        int *resultados = malloc(MAX * sizeof(int));
        int cantidad = 0;
        char buffer[BUFFER_SIZE];
        int a, b;

        while (read(fd, buffer, BUFFER_SIZE) > 0 && cantidad < MAX) {
            sscanf(buffer, "%d %d", &a, &b);
            int resultado_mul = a * b;
            resultados[cantidad] = resultado_mul;
            cantidad++;

            shmatch->a[shmatch->n_mul] = a;
            shmatch->b[shmatch->n_mul] = b;
            shmatch->resultado_mul[shmatch->n_mul] = resultado_mul;
            shmatch->n_mul++;

            printf("Hijo 2 Multiplicacion: PID %d, %d * %d = %d\n", getpid(), a, b, resultado_mul);
            fflush(stdout);
        }

        free(resultados);
        close(fd);
        shmdt(shmatch);
        exit(0);
    }

    wait(NULL);
    wait(NULL);
    wait(NULL);

    printf("\nPadre: resultados en memoria compartida\n");
    for (int i = 0; i < shmatch->n_mul; i++) {
        printf("(%d, %d) suma = %d, multiplicacion = %d\n",
               shmatch->a[i], shmatch->b[i],
               shmatch->resultado_sum[i], shmatch->resultado_mul[i]);
    }

    shmdt(shmatch);
    if (shmctl(shm_id, IPC_RMID, NULL) < 0) {
        perror("Error eliminando memoria compartida");
        exit(1);
    }

    unlink(FIFO_SUMA);
    unlink(FIFO_MUL);

    printf("Padre: mis hijos terminaron\n");
    return 0;
}
