#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "pixel2ascii.h"
#include "timeprofiler.h"

#define N 5


char* buffer1;
char* buffer2;
char *array[2];

//int writeID;
int nextImageId;
int rightWorker;

pthread_t worker_thread[N];
pthread_t IRequester_thread;
int id[N];
int reqId;
int readPos;
int writePos;
int cicle;
int nLines;

sem_t canRead;
sem_t canWrite;
sem_t nextImage;
sem_t newImage;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t mutex2 = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t go_on = PTHREAD_COND_INITIALIZER;

FILE *fpin;
int col;
int line;

//variavel q guarda o nome da imagem em questao
char filepath[256];

void shutdown() {
    int i;
    //waits for them to die
    for (i = 0; i < N; i++) {
        pthread_join(worker_thread[i], NULL);
        printf("Waiting for worker...\n");
    }
    pthread_join(IRequester_thread, NULL);
    printf("Waiting for requester...\n");

    printf("Closing file pointers.\n");
    fclose(fpin);
    printf("\nClean shutdown completed!!!!!\n");
    exit(0);
}

void ctrlc() {
    char c[2];
    printf("\nYou really want to terminate (y/n)?\n");
    scanf("%s", c);

    if (strcmp(c, "y") == 0) {
        kill(0, SIGTERM);
    }
}

void terminate(int signum) {
    ctrlc();
}

void *worker(void* idp) {

    int my_id = *(int *) idp; //Debug purposes
    signal(SIGINT, terminate);
    pixel* image;
    int i;

    char path[256];

    FILE *fp;

    while (cicle) {

        sem_wait(&newImage); //esperam ate haver alguma imagem

        pthread_mutex_lock(&mutex2);
        int ID = nextImageId;
        nextImageId++;
        strcpy(path, filepath);
        pthread_mutex_unlock(&mutex2);

        sem_post(&nextImage);

        fp = fopen(path, "r");
        if (fpin == NULL) {
            printf("Could not open input file\n");
            return NULL;
        }

        // printf("Getting header\n");
        header* h = getImageHeader(fp);

        if (h == NULL) {
            printf("Error getting header from file\n");
            return NULL;
        }

        //alloc mem space for image
        image = malloc(h->width * h->height * sizeof (pixel));

        //read image to a linear pixel array	
        for (i = 0; i < h->height * h->width; i++) {
            if (fread(&image[i], sizeof (byte), 3 * sizeof (byte), fp) < 1) {
                printf("Error while reading image\n");
                return NULL;
            }
        }

        //get ascii array resulting from conversion
        char *asciiImage = pixel2ASCII(image, h->width, h->height, col, line);

        sem_wait(&canWrite);
        pthread_mutex_lock(&mutex);

        while (ID != rightWorker) {
            sem_post(&canWrite);
            pthread_cond_wait(&go_on, &mutex);

        }

        array[writePos] = asciiImage;
        writePos = (writePos + 1) % 2;

        printf("FRAME: %d | THREAD ID: %d | WRITE POS: %d\n", ID + 1, my_id, writePos);
        pthread_mutex_unlock(&mutex);
        sem_post(&canRead);

    }

    //clean up image
    free(image);
    fclose(fp);

    pthread_exit(NULL);
    return NULL;
}

void *requester(void* idp2) {


    signal(SIGINT, terminate);

    while (fscanf(fpin, "%s", filepath) != EOF) {//conta as linhas
        nLines++;
    }

    rewind(fpin); //volta ao inicio do ficheiro

    while (1) {
        sem_wait(&nextImage);

        if (fscanf(fpin, "%s", filepath) == EOF) {
            break;
        }

        sem_post(&newImage); //diz que ha uma imagem
    }

    cicle = 0;
    pthread_exit(NULL);
    return NULL;
}

void init() {
    sem_init(&canRead, 0, 0);
    sem_init(&canWrite, 0, 2);
    sem_init(&nextImage, 0, 1);
    sem_init(&newImage, 0, 0);

    writePos = readPos = nextImageId = rightWorker = 0;
    nLines = 1;
    reqId = 10;
    cicle = 1;
}

int main(int argc, char *argv[]) {
    int i;
    double startms, stopms;

    signal(SIGINT, terminate);
    init();

    //check parameters
    if (argc < 4) {
        printf("Incorrect usage.\nPlease use \"./main input_filename.ppm charWidth charHeight \"\n");
        return -1;
    }

    printf("Opening input file [%s]\n", argv[1]);
    fpin = fopen(argv[1], "r");
    if (fpin == NULL) {
        printf("Could not open input file\n");
        return -1;
    }

    //get final size values from command line args
    col = atoi(argv[2]);
    line = atoi(argv[3]);

    //pede espaço para os buffers
    int tam = atoi(argv[2]) * atoi(argv[3]);
    char* buffer1 = malloc(tam * sizeof (char));
    char* buffer2 = malloc(tam * sizeof (char));

    array[0] = buffer1;
    array[1] = buffer2;

    // create request thread
    pthread_create(&IRequester_thread, NULL, requester, &reqId);
    // create N threads
    for (i = 0; i < N; i++) {
        id[i] = i;
        pthread_create(&worker_thread[i], NULL, worker, &id[i]);
    }

    i = 0;
    startms = getCurrentTimeMili();

    struct timespec tim;
    tim.tv_sec = 0;
    tim.tv_nsec = 500000000L;

    while (1) {

        i++;

        sem_wait(&canRead);
        pthread_mutex_lock(&mutex);

        //clean screan before starting to draw ascii art
        //clearConsole();
        nanosleep(&tim, &tim);
        printAsciiImage(array[readPos], col);
        readPos = (readPos + 1) % 2;

        rightWorker++; //proximo worker a escrever no buffer

        pthread_mutex_unlock(&mutex);
        pthread_cond_broadcast(&go_on);

        sem_post(&canWrite);

        if (nLines == i + 1) {
            break;
        }

    }


    stopms = getCurrentTimeMili();
    printf("\n");
    printf("Total time: %g\n", (stopms - startms) / 1000);
    printf("Image number: %d\n", nextImageId);
    printf("FPS: %g\n", nextImageId / ((stopms - startms) / 1000));

    shutdown();
    return 0;
}
