#include "vars.h"

//funcao que ira calcular o tempo decorrido

double getEllapsedTime(struct timeval start, struct timeval end) {
    //convert everything to microseconds
    double tStart = start.tv_sec * 1000000.0 + start.tv_usec;
    double tEnd = end.tv_sec * 1000000.0 + end.tv_usec;

    return tEnd - tStart;
}

void shutdown() {
    char c[2];
    printf("Are you sure to terminate (y/n)?\n");
    scanf("%s", c);

    if (strcmp(c, "y") == 0) {

        free(fd);
        free(fd2);

        //terminar e matar maps
        printf("\nExiting............\n\n");
        kill(reduce_id, SIGTERM);
        printf("Reduce[%d] exited...\n", reduce_id);


        for (int i = 0; i < nMaps; i++) {
            kill(id[i], SIGTERM);
            printf("Map[%d] exited...\n", id[i]);

        }

        free(id);
    } else {
        printf("\\>");
    }

}

void erro(int signum) {
    printf("Error detected.\nPlease correct it and try again...\n\\>");
}


//mostra os comandos disponiveis

void show_available_commands() {
    printf("\nAvailable commands:\n");
    printf("\\> search filename lower_bound_ip upper_bound_ip\n");
    printf("\\> exit\n\n");
}

void terminate(int signum) {
    shutdown();
    printf("[%d] Master exiting....\n", getpid());
    printf("Clean shutdown completed!\n");
    exit(0);
}

//pai recebe sinal do reduce, e imprime o tempo decorrido

void sucesso(int signum) {

    gettimeofday(&end, NULL); //getting timestamp of the end

    double dif = getEllapsedTime(start, end); //calculate diference using given function

    //print result in microseconds with no cases (none needeed)
    printf("Time taken to process log file: %.0f microseconds.\n\\>", dif);
}


//processo reduce, soma a info enviada pelos maps, imprime e manda sinal ao pai 

void reduce() {
    //signal(SIGINT,terminate_childs);
    int i, leituras = 0, conta = 0, final_conta = 0;


    //fechar fd desnecessarios
    for (i = 0; i < nMaps; i++) {
        close(fd2[i][1]);
    }

    signal(SIGINT, terminate);

    sigset_t block_ctrlc;
    sigemptyset(&block_ctrlc);
    sigaddset(&block_ctrlc, SIGINT);

    sigprocmask(SIG_BLOCK, &block_ctrlc, NULL);

    /*********************select**************************/
    while (1) {
        //final_conta=0;
        fd_set read_set;
        FD_ZERO(&read_set);

        for (i = 0; i < nMaps; i++) {
            FD_SET(fd2[i][0], &read_set);
        }

        if (select((int) fd2[nMaps - 1] + 1, &read_set, NULL, NULL, NULL) > 0) //ultimo fd+1
        {

            for (i = 0; i < nMaps; i++) {

                //printf("leu do %d\n",i);
                if (FD_ISSET(fd2[i][0], &read_set)) {
                    int n, total = 0;
                    while (total<sizeof (int)) {//proteçao

                        n = read(fd2[i][0], &conta + total, sizeof (int)-total); //ler a estrutura com os dados

                        if (n == -1) {
                            if (errno == EAGAIN)
                                continue;
                            else {
                                printf("\nError reading from pipe!\n");
                                exit(0);
                            }
                        }

                        total += n;
                    }

                    leituras++; //var q controla se ja lemos a info d todos os maps		
                    final_conta += conta;
                }

            }

            if (conta < 0) {
                error_flag = 1;
            }

            if (leituras == nMaps)//se ja fez 1 leitura de cada map, avisa o pai
            {

                if (error_flag == 0) {
                    printf("\n%d matches found.\n", final_conta);
                    leituras = 0;
                    final_conta = 0;
                    error_flag = 0;
                    kill(master_id, SIGUSR1);
                } else {
                    error_flag = 0;
                    leituras = 0;
                    final_conta = 0;
                    kill(master_id, SIGUSR2);


                }

            }
        }
    }


    sigprocmask(SIG_UNBLOCK, &block_ctrlc, NULL);
}

int checkIP(int* ip) {
    int i;
    for (i = 0; i < 4; i++) {
        //invalid ip field
        if (ip[i] < 0 || ip[i] > 255) {
            return -1;
        }
    }

    return 0;
}

//recursive function that compares the given ip with the lower and higher bounds
//it returns 0 if the ip is between limits
//it returns -1 is the ip is lower than limits
//it returns 1 if the ip is higher than limits

int compareIP(int* low, int* high, int* ip, int i) {
    //if index is 4, there is no more comparisons to do and the ip is equal to low limit or high limit and should be counted
    if (i == 4) {
        return 0;
    }

    //if value in current index is lower than the low limit, then ip us out of range
    if (ip[i] < low [i]) {
        return -1;
    }//if value in current index is higher than the high limit, the ip is out of range
    else if (ip[i] > high[i]) {
        return 1;
    }//check next value if equals to lower field
    else if (ip[i] == low[i]) {
        //if the response is -1, the ip is out of range
        if (compareIP(low, high, ip, i + 1) == -1) {
            return -1;
        } else {
            return 0;
        }
    }//check next value if equals to high field
    else if (ip[i] == high[i]) {
        //if the response is 1, the ip is out of range
        if (compareIP(low, high, ip, i + 1) == 1) {
            return 1;
        } else {
            return 0;
        }
    }//the value in current index the ip is between the limits, it is in range
    else {
        return 0;
    }
}

//conta as vezes que encontrou (tem q receber id do map em questao, para ler as linhas respectivas)

int count(char* filename, int* lowLimit, int* highLimit, int id_maps) {
    FILE* logFile = fopen(filename, "r");

    if (logFile == NULL) {
        printf("File %s does not exist.\n", filename);
        return -1;
    }

    //check user input
    if (checkIP(lowLimit) != 0) {
        printf("Invalid lower bound ip.\n");
        return -1;
    }
    if (checkIP(highLimit) != 0) {
        printf("Invalid higher bound ip.\n");
        return -1;
    }


    int count = 0, ip[4];

    char line[1024];

    int conta_linha = -1;

    //for each linedividir
    while (fgets(line, 1024, logFile) != NULL) {
        conta_linha++;
        if ((conta_linha % nMaps) == id_maps) {

            //Read the ip from the line
            if (sscanf(line, "%i.%i.%i.%i", &ip[0], &ip[1], &ip[2], &ip[3]) != 4) {
                continue;
            }

            if (compareIP(lowLimit, highLimit, ip, 0) == 0) {
                count++;
            }
        }

    }

    fclose(logFile);

    return count;
}

void map(int i)//rotina que vai contar o nMapsero de ips encontrados
{
    info stuff;
    int conta;

    signal(SIGINT, terminate);

    sigset_t block_ctrlc;
    sigemptyset(&block_ctrlc);
    sigaddset(&block_ctrlc, SIGINT);

    close(fd[i][1]); //fecha fd desnecessarios
    close(fd2[i][0]);

    sigprocmask(SIG_BLOCK, &block_ctrlc, NULL);

    while (1) {

        int n, total = 0;

        while (total<sizeof (info)) {//proteçao

            n = read(fd[i][0], &stuff + total, sizeof (info) - total);
            if (n == -1) {
                if (errno == EAGAIN)
                    continue;
                else {
                    printf("\nError reading from pipe!\n");
                    exit(0);
                }
            }
            total += n;
        }

        conta = count(stuff.filename, stuff.lowLimit, stuff.highLimit, stuff.id_map);
        write(fd2[i][1], &conta, sizeof (int)); //escreve para o reduce

    }
    sigprocmask(SIG_UNBLOCK, &block_ctrlc, NULL);
}

void create_reduce() {

    if ((reduce_id = fork()) == 0)//inicializa o reduce
    {
        reduce();
    }
}

void create_maps() {
    //cria X processos map, e guarda os ID's d cada um
    for (int i = 0; i < nMaps; i++) {

        if ((id[i] = fork()) == 0)//retorna o pid do processo (pq ta no processo original)
        {
            //cria os maps e mete-os a espera de trabalho
            map(i);
            exit(0);

        }
    }
}

int main(int argc, char *argv[]) {
    //routine to handle signal
    signal(SIGUSR1, sucesso);
    signal(SIGUSR2, erro);
    signal(SIGINT, terminate);

    //signal(SIGINT, terminate);

    sigset_t block_ctrlc;
    sigemptyset(&block_ctrlc);
    sigaddset(&block_ctrlc, SIGINT);

    info information;
    int i;

    //command input and filename
    char cmd[100];

    //control var
    int exits = 1;

    printf("How many maps do you want to create?\n");
    scanf("%d", &nMaps);

    //alocaçao dinamica para Upipes
    fd = (int**) malloc(nMaps * sizeof (int*));
    fd2 = (int**) malloc(nMaps * sizeof (int*));

    id = (pid_t*) malloc(nMaps * sizeof (pid_t));

    // Verificar se a alocacao de memoria foi bem sucedida
    if (!fd || !fd2) {
        printf("Failed to allocate memory for pipes!\n");
        exit(0);
    }

    // Aloca memoria para as colunas da matriz
    for (i = 0; i < nMaps; i++) {
        fd[i] = (int*) malloc(2 * sizeof (int*));
        fd2[i] = (int*) malloc(2 * sizeof (int*));
    }

    show_available_commands();


    //cria os unnamed pipes
    for (i = 0; i < nMaps; i++) {
        pipe(fd[i]); //pipe master-map
        pipe(fd2[i]); //pipe map-reduce
    }


    master_id = getpid(); //guarda id do pai

    create_reduce();
    create_maps();


    for (i = 0; i < nMaps; i++) {//fecha fd desnecessarios
        close(fd[i][0]);
    }

    //shell loop
    printf("\\> ");
    while (exits) {

        //get command
        scanf("%s", cmd);

        if (strcmp(cmd, "exit") == 0) {
            exits = 0;
            shutdown();
        } else if (strcmp(cmd, "search") == 0) {
            //get aditional command vars
            scanf("%s", information.filename);
            scanf("%i.%i.%i.%i", &information.lowLimit[0], &information.lowLimit[1], &information.lowLimit[2], &information.lowLimit[3]);
            scanf("%i.%i.%i.%i", &information.highLimit[0], &information.highLimit[1], &information.highLimit[2], &information.highLimit[3]);

            printf("Starting to search on \"%s\". Please wait...\n", information.filename);
            gettimeofday(&start, NULL); //getting timestamp of the start

            sigprocmask(SIG_BLOCK, &block_ctrlc, NULL);

            for (i = 0; i < nMaps; i++) {
                information.id_map = i;
                write(fd[i][1], &information, sizeof (information));
            }

            pause(); //fica à espera dum sinal do reduce
            sigprocmask(SIG_UNBLOCK, &block_ctrlc, NULL);

        } else {
            printf("Command not recognised...\n");
            show_available_commands();
        }
    }


    return 0;

}

