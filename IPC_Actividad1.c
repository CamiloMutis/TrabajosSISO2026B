#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <sys/shm.h>

#define FIFO_FILE "canal_fifo"
#define BUFFER_SIZE 128

struct Datos {
    int a;
    int b;
    int resultado_sum;
    int resultado_mul;
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

    mkfifo(FIFO_FILE, 0666);

    printf("Padre: PID %d\n", getpid());
    fflush(stdout);

    // Hijo 1: envia los datos por el FIFO
    if (fork() == 0) {
        int fd = open(FIFO_FILE, O_WRONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO para escritura");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        snprintf(buffer, BUFFER_SIZE, "%d %d", 4, 6);
        write(fd, buffer, strlen(buffer) + 1);
        printf("Hijo 1: PID %d, enviado por FIFO: %s\n", getpid(), buffer);

        close(fd);
        shmdt(shmatch);
        exit(0);
    }

    // Hijo 2: lee del FIFO, calcula y guarda en memoria compartida
    if (fork() == 0) {
        int fd = open(FIFO_FILE, O_RDONLY);
        if (fd < 0) {
            perror("Error abriendo FIFO para lectura");
            exit(1);
        }

        char buffer[BUFFER_SIZE];
        read(fd, buffer, BUFFER_SIZE);
        sscanf(buffer, "%d %d", &shmatch->a, &shmatch->b);

        shmatch->resultado_sum = shmatch->a + shmatch->b;
        shmatch->resultado_mul = shmatch->a * shmatch->b;

        printf("Hijo 2: PID %d, recibido %d y %d, guardado en memoria compartida\n",
               getpid(), shmatch->a, shmatch->b);

        close(fd);
        shmdt(shmatch);
        exit(0);
    }

    wait(NULL);
    wait(NULL);

    printf("Padre: %d + %d = %d\n", shmatch->a, shmatch->b, shmatch->resultado_sum);
    printf("Padre: %d * %d = %d\n", shmatch->a, shmatch->b, shmatch->resultado_mul);

    shmdt(shmatch);
    if (shmctl(shm_id, IPC_RMID, NULL) < 0) {
        perror("Error eliminando memoria compartida");
        exit(1);
    }
    unlink(FIFO_FILE);

    printf("Padre: mis hijos terminaron\n");
    return 0;
}
