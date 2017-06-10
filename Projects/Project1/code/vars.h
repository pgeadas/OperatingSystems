#include <stdio.h>
#include <stdlib.h>
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

//Pedro Miguel Ricardo Geadas

int **fd; //unnamed Pipes
int **fd2;
int nMaps;//maps number

int error_flag = 0;

pid_t *id;
pid_t reduce_id;
pid_t master_id;

//struct for time control
struct timeval start, end;

typedef struct {
	int lowLimit[4];
	int highLimit[4];
	char filename[100];
	int id_map;
} info;
