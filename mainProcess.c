#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <math.h>

//Shared memory
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include "shared_memory.h"

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define PIPE_BUFF 512
#define SLAVE_HASH_OUTPUT 106 //Counts it as a string.

// Data structure useful for creating a group of pipes used by the main process
typedef struct pipeGroup{
    int **pipes;
} pipeGroup;




int main(int argc, char *argv[]) {
    // argsv[] = {"./mainProcess", "./archivo1", "./archivo2"} 
    // argc = mainPath + #arguments

    // Pipes used for writing on the slave processes
    pipeGroup inPipes;

    // Pipes used for read the slave processes results
    pipeGroup outPipes;

    int status;

    // The program path/name doesn't count, so we start to counting one after
    int argCount = 1;

    // If the program has less paths than the default slaves amount, it only creates the necesary amount of slaves
    
    /* Slaves are created in a log2 order of input, it reduces drastically 
    ** the amount of processes created for each group of files
    */ 
    int slavesAmount = (int) log2((double) argc-1);
    int initialPaths = slavesAmount / 2;

    /* If we receive only 1 argument -> log2(1) = 0, so we need to increment slavesAmount 
    **for the program to have at least 1 slave
    */
    if(slavesAmount < 1){
        slavesAmount++;
        initialPaths++;
    }

    inPipes.pipes = malloc(sizeof(int*)*slavesAmount);
    outPipes.pipes = malloc(sizeof(int*)*slavesAmount);

    if(inPipes.pipes == NULL || outPipes.pipes == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    // We now create all the pipes that we're going to need
    for(int i=0; i < slavesAmount ;i++){
        inPipes.pipes[i] = malloc(sizeof(int)*2);
        outPipes.pipes[i] = malloc(sizeof(int)*2);
        if(inPipes.pipes[i] == NULL || outPipes.pipes[i] == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        if(pipe(inPipes.pipes[i]) == ERROR || pipe(outPipes.pipes[i]) == ERROR){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid;
    for (int i = 0; i < slavesAmount; i++) {
        if((pid = fork()) == ERROR){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){
            int aux;
            //close(0)
            //dup(inPipes.pipes[i][0])      r-end del pipe
            if((aux = dup2(inPipes.pipes[i][0], STDIN)) == ERROR || aux != STDIN){
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            if((aux = dup2(outPipes.pipes[i][1], STDOUT)) == ERROR || aux != STDOUT){        //w-end del pipe
                perror("dup");
                exit(EXIT_FAILURE);
            }

            // No need for the pipes variables to exist in the child process
            // We can't have memory leaks anywhere, so we're freeing everything that we are not using
            // and closing every fd that we are not using
            for(int j=0; j<slavesAmount ;j++){
                if(close(inPipes.pipes[j][0]) == ERROR || close(inPipes.pipes[j][1]) == ERROR 
                        || close(outPipes.pipes[j][0]) == ERROR || close(outPipes.pipes[j][1])){
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                free(inPipes.pipes[j]);
                free(outPipes.pipes[j]);
            }
            free(inPipes.pipes);
            free(outPipes.pipes);

            // Everything is ready for the slaves processes
            char *const argvC[] = {"./slave", NULL};
            char *envpC[] = {NULL};
            execve("./slave", argvC, envpC);
            
            //If reached this line, is because it was an error.
            perror("execve");
            exit(EXIT_FAILURE);
        }
    }

    // We need to close the rest of the pipes in the parent code before forking of doing anything else
    for(int i=0; i<slavesAmount ;i++){
        if(close(inPipes.pipes[i][0]) == ERROR || close(outPipes.pipes[i][1]) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }
    }

    // Now we have to distribute evenly all the initial paths that the slaves have to process
    char writeBuff[PIPE_BUFF];
    for(int i=0; i<initialPaths && argCount < argc ; i++)
        for(int j=0; j<slavesAmount && argCount < argc; j++, argCount++){
            sprintf(writeBuff, "%s\n", argv[argCount]);
            write(inPipes.pipes[j][1], writeBuff, strlen(writeBuff));
        }

    // Now we can create a subprocess of the main program that can read all the data given by the slaves
    if((pid = fork()) == ERROR){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    
    if(pid == 0){
        /*
        La vista esperará el output, que recién en este momento se hace.
        */
        size_t shared_memory_size = (argc-1) * SLAVE_HASH_OUTPUT + 1024;
        shared_memory_ADT shm = initialize_shared_memory(shared_memory_size);
        printf("%lu\n", shared_memory_size);
        int output_fd;
        if((output_fd=open("output.txt", O_WRONLY | O_CREAT)) == ERROR){
            perror("open");
            exit(EXIT_FAILURE);
        }

        // We don't need the writing pipes if we are in this process
        for(int i=0; i<slavesAmount; i++){
            if(close(inPipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(EXIT_FAILURE);
            }
            free(inPipes.pipes[i]);
        }
        free(inPipes.pipes);

        // Now we need to make all the synchronization process for reading all the outputs from the slaves
        // is blocked waiting for the childs to be on ready state
        fd_set readfd;
        int fdAmount, ndfs;

        ndfs = 0;
        for(int i=0; i < slavesAmount ;i++)
            ndfs = outPipes.pipes[i][0] > ndfs ? outPipes.pipes[i][0] : ndfs;
        ndfs++;

        ssize_t charsRead = 1;
        char buffer[PIPE_BUFF] = {0};
        int *closedPipes = calloc(slavesAmount, sizeof(int));
        if(closedPipes == NULL){
            perror("calloc");
            exit(EXIT_FAILURE);
        }
        int closedPipesAmount = 0;

        // We need to process the rest of the programs arguments
        while(closedPipesAmount < slavesAmount){
            // Initialize the File Descriptor set for the use of select
            FD_ZERO(&readfd);
            for(int i=0; i<slavesAmount ;i++){
                if(closedPipes[i] != 1)
                    FD_SET(outPipes.pipes[i][0], &readfd);
            }

            // Using pselect and unmasking the childs should let the program free of race conditions
            if((fdAmount = select(ndfs, &readfd, NULL, NULL, NULL)) == ERROR){
                perror("select");
                exit(EXIT_FAILURE);
            }
            // No slave processes free, should never happen because we're not using a timeout condition
            if(fdAmount == 0) continue;


            // For every process that is ready, we can write on it the next file that needs to be processed
            for(int i=0; i < slavesAmount && fdAmount > 0 ;i++){
                if(closedPipes[i] != 1 && FD_ISSET(outPipes.pipes[i][0], &readfd)){
                    charsRead = read(outPipes.pipes[i][0], buffer, PIPE_BUFF);
                    
                    // EOF reached on that pipe
                    if(charsRead == 0){
                        if(close(outPipes.pipes[i][0]) == ERROR){
                            perror("close2");
                            exit(EXIT_FAILURE);
                        }
                        free(outPipes.pipes[i]);
                        closedPipes[i] = 1;
                        closedPipesAmount++;
                    } else{
                        int length = strlen(buffer);
                        //printf("->%s<-\n", buffer);

                        write_to_shared_memory(shm, buffer, length);
                        write(output_fd, buffer, length);

                        for(int i=0; buffer[i] != '\0' ;i++) buffer[i] = '\0';
                        // Here we need to put the line read on the final file and the shared memory space
                        //after we implement the shared memory program                    

                    }
                    fdAmount--;
                }
            }
        }

        // Every pipe is closed, now we need to free the remaining memory spaces and finish this process
        free(outPipes.pipes);
        free(closedPipes);

        // We notify to the shared memory that we finished writing in it
        strcpy(buffer, "$");
        write_to_shared_memory(shm, buffer, strlen(buffer));
        
        sleep(2);
        //show_shared_memory(shm);
        unlink_shared_memory_resources(shm);
        if(close(output_fd) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }

        return EXIT_SUCCESS;
    }

    // If we are the mainProcess, we still need to keep writing the remaining paths evenly on the slaves' pipes
    // But first, we need to get rid of the pipes that we don't need
    for(int i=0; i<slavesAmount ;i++){
        if(close(outPipes.pipes[i][0]) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }
        free(outPipes.pipes[i]);
    }
    free(outPipes.pipes);

    fd_set writefd;
    int fdAmount, ndfs;

    ndfs = 0;
    for(int i=0; i < slavesAmount ;i++)
        ndfs = inPipes.pipes[i][1] > ndfs ? inPipes.pipes[i][1] : ndfs;
    ndfs++;

    // We need to process the rest of the programs arguments
    while(argCount < argc){
        // Initialize the File Descriptor set for the use of select
        FD_ZERO(&writefd);
        for(int i=0; i<slavesAmount ;i++){
            FD_SET(inPipes.pipes[i][1], &writefd);
        }

        // Using pselect and unmasking the childs should let the program free of race conditions
        if((fdAmount = select(ndfs, NULL, &writefd, NULL, NULL)) == ERROR){
            perror("select");
            exit(EXIT_FAILURE);
        }
        // No slave processes free, should't happen
        if(fdAmount == 0) continue;
        // For every process that is ready, we can write on it the next file that needs to be processed
        for(int i=0; i < slavesAmount && argCount < argc;i++){
            if(FD_ISSET(inPipes.pipes[i][1], &writefd)){
                sprintf(writeBuff,"%s\n",argv[argCount]);
                
                write(inPipes.pipes[i][1], writeBuff, strlen(writeBuff));

                argCount++;
            }    
            
        }
    }

    // No arguments left, now we need to close the pipes and free the heap
    for(int i=0; i < slavesAmount ;i++){
        if(close(inPipes.pipes[i][1]) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }
        free(inPipes.pipes[i]);
    }
    free(inPipes.pipes);

    // Waits until ALL child processes finishes their tasks
    while(waitpid(-1, &status, 0) > 0);
    
    //wait(&status);
    if(status != 0){
        perror("status of some child");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}