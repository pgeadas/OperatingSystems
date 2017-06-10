#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <semaphore.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/shm.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/fcntl.h>
#include <sys/wait.h>

#include <sys/mman.h>
#include "timeprofiler.h"
#include "ppmtools.h"

header *sh_mem;

void shutdown() {
    char c[2];
    printf("Are you sure you want to exit (y/n) ?\n");
    scanf("%s", c);

    if (strcmp(c, "y") == 0) {
        printf("Removing shared memory...\n");

        shmctl(sh_mem->shmid, IPC_RMID, NULL);

        printf("Removing semaphores...\n");
        sem_close(sh_mem->sem_remaining_lines);
        sem_unlink("SEM_REMAINING_LINES");
        sem_close(sh_mem->mutex2);
        sem_unlink("MUTEX2");
        sem_close(sh_mem->mutex3);
        sem_unlink("MUTEX3");

        printf("Terminating processes...\n");
        kill(0, SIGTERM);
    }


}

int main(int argc, char **argv) {
    int shmid, N, i;

    double start, stop, startms, stopms;
    sem_t *mutex2, *mutex3, *sem_remaining_lines;

    signal(SIGINT, shutdown);

    sigset_t block_ctrlc;
    sigemptyset(&block_ctrlc);
    sigaddset(&block_ctrlc, SIGINT);

    sigprocmask(SIG_BLOCK, &block_ctrlc, NULL);


    if (argc < 3)//args from command line
    {
        printf("Incorrect usage.\nPlease use \"./invert input_filename.ppm output_filename.ppm\"\n");
        return -1;
    }


    //tries to open origin and dest files
    printf("Opening input file [%s]\n", argv[1]);
    FILE *fpin = fopen(argv[1], "r");
    if (fpin == NULL) {
        printf("Could not open input file\n");
        return -1;
    }


    printf("Opening output file [%s]\n", argv[2]);
    FILE *fpout = fopen(argv[2], "w");
    if (fpout == NULL) {
        printf("Could not open output file\n");
        return -1;
    }


    //if it reads the info from image correctly
    printf("Getting header\n");
    header *h = getImageHeader(fpin);

    if (h == NULL) {
        printf("Error getting header from file\n");
        return -1;
    }
    printf("Got file Header: %s - %u x %u - %u\n", h->type, h->width, h->height, h->depth);
    h->nextline = -1;

    //grava a info
    printf("Saving header to output file\n");
    if (writeImageHeader(h, fpout) == -1) {
        printf("Could not write to output file\n");
        return -1;
    }

    //inicializa semaforos
    sem_unlink("MUTEX2");
    mutex2 = sem_open("MUTEX2", O_CREAT | O_EXCL, 0700, 0);

    sem_unlink("MUTEX3");
    mutex3 = sem_open("MUTEX3", O_CREAT | O_EXCL, 0700, 1);

    sem_unlink("SEM_REMAINING_LINES");
    sem_remaining_lines = sem_open("SEM_REMAINING_LINES", O_CREAT | O_EXCL, 0700, 0);

    printf("How many workers do you want to create?\n");
    scanf("%d", &N);


    //creates a shared memory segment of the size of mem_sctruct
    shmid = shmget((key_t) 1300, sizeof (header), IPC_CREAT | 0700);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }

    //maps the shared memory in the address space of the current process
    sh_mem = (header*) shmat(shmid, NULL, 0);
    if (sh_mem == (header *) - 1) {
        perror("shmat");
        exit(0);
    }

    printf("Memory connected at %x\n", (int) sh_mem);

    //puts semaphore info in shared memmory
    sh_mem->mutex2 = mutex2;
    sh_mem->mutex3 = mutex3;
    sh_mem->sem_remaining_lines = sem_remaining_lines;
    sh_mem->shmid = shmid;


    //start timer
    start = getCurrentTimeMicro();
    startms = getCurrentTimeMili();

    //create workers
    create_workers(N);


    //copies info to shared memory
    strcpy(sh_mem->type, h->type);
    sh_mem->width = h->width;
    sh_mem->height = h->height;
    sh_mem->depth = h->depth;
    sh_mem->nextline = -1;

    for (i = 0; i < h->height; i++) {
        printf("Reading row %d ... ", i + 1);
        if (getImageRow(sh_mem->width, sh_mem->table[i], fpin) == -1) {
            printf("Error while reading row\n");
            return -1;
        }
        printf("|| Got row to shared memory %d || \n", (i + 1));

        sem_post(sh_mem->sem_remaining_lines);

    }

    sem_wait(sh_mem->mutex2);

    //stop timer
    stop = getCurrentTimeMicro();
    stopms = getCurrentTimeMili();
    printTimeElapsed(start, stop, "microseconds");
    printTimeElapsed(startms, stopms, "miliseconds");


    for (i = 0; i < sh_mem->height; i++) {
        if (writeRow(sh_mem->width, sh_mem->table[i], fpout) == -1) {
            printf("Error while reading row\n");
            return -1;
        }
    }

    printf("Removing shared memory...\n");

    shmctl(shmid, IPC_RMID, NULL);

    printf("Removing semaphores...\n");
    sem_close(sh_mem->sem_remaining_lines);
    sem_unlink("SEM_REMAINING_LINES");
    sem_close(sh_mem->mutex2);
    sem_unlink("MUTEX2");
    sem_close(sh_mem->mutex3);
    sem_unlink("MUTEX3");


    printf("Terminating processes...\n");
    kill(0, SIGTERM);

    return 0;

}

int worker() {

    int shmid;
    int line;
    signal(SIGINT, shutdown);

    sigset_t block_ctrlc;
    sigemptyset(&block_ctrlc);
    sigaddset(&block_ctrlc, SIGINT);

    //cria mem partilhada
    shmid = shmget((key_t) 1300, sizeof (header), 0700);
    if (shmid < 0) {
        perror("shmget");
        exit(1);
    }
    sh_mem = (header *) shmat(shmid, NULL, 0);

    if (sh_mem == (header *) - 1) {
        perror("shmat");
        exit(1);
    }

    printf("Memory connected at %x\n", (int) sh_mem);


    while (1) {
        sem_wait(sh_mem->sem_remaining_lines);
        sem_wait(sh_mem->mutex3);
        line = (sh_mem->nextline) += 1;
        sem_post(sh_mem->mutex3);

        printf("Inverting row %d ...", line + 1);
        invertRow(sh_mem->width, sh_mem->table[line]);
        printf("|| Done !\n");

        if (line == (sh_mem->height) - 1) {
            sem_post(sh_mem->mutex2);
            sigprocmask(SIG_UNBLOCK, &block_ctrlc, NULL);
            break;
        }

    }

    shmctl(shmid, IPC_RMID, NULL);

    return 0;

}

void create_workers(int N) {
    int i;
    //create childs
    for (i = 0; i < N; i++) {
        if (fork() == 0) {
            worker();
            printf("Child exiting...\n");
            exit(0);
        }
    }
}

