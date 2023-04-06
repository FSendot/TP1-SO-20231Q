#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define MAX_PER_SLAVE 3
#define INITIAL_SLAVES 5

// Data structure useful for creating a group of pipes used by the main process
typedef struct pipeGroup{
    int **pipes;
} pipeGroup;

int main(int argc, char argv[]) {
    // argsv[] = {"./mainProcess", "./archivo1", "./archivo2"} 
    // argc = mainPath + #arguments
    pipeGroup inPipes;
    pipeGroup outPipes;

    int status;

    // The program path/name doesn't count, so we start to counting one after
    int argCount = 1;

    // If the program has less paths than the default slaves amount, it only creates the necesary amount of slaves
    
    /* If we wanted to add complexity to the amount of slaves
    ** created, the code below can support a variable amount
    ** of slaves, thanks to the heap, we can calculate everything 
    ** in real time.
    */
    int slavesAmount = (argc - 1) < INITIAL_SLAVES ? (argc - 1) : INITIAL_SLAVES;

    inPipes.pipes = malloc(sizeof(int*)*slavesAmount);
    outPipes.pipes = malloc(sizeof(int*)*slavesAmount);

    for (int i = 0; i < slavesAmount; i++) {
        inPipes.pipes[i] = malloc(sizeof(int)*2);
        outPipes.pipes[i] = malloc(sizeof(int)*2);
        if(pipe(inPipes.pipes[i]) == ERROR){
            perror("pipe");
            exit(1);
        }
        if(pipe(outPipes.pipes[i]) == ERROR){
            perror("pipe");
            exit(1);
        }

        pid_t pid;
        if((pid = fork()) == ERROR){
            perror("fork");
            exit(1);
        }
        if(pid == 0){
            if(dup2(inPipes.pipes[i][0], STDIN) == ERROR){
                perror("dup2");
                exit(1);
            }
            if(close(inPipes.pipes[i][0]) == ERROR ||close(inPipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(1);
            }

            if(dup2(outPipes.pipes[i][1], STDOUT) == ERROR){
                perror("dup2");
                exit(1);
            }
            if(close(outPipes.pipes[i][1]) == ERROR || close(outPipes.pipes[i][0]) == ERROR){
                perror("close");
                exit(1);
            }

            // No need for the pipes variables to exist in the child process
            // We can't have memory leaks anywhere, so we're freeing everything that we are not using
            for(int j=0; j<slavesAmount ;j++){
                free(inPipes.pipes[i]);
                free(outPipes.pipes[i]);
            }
            free(inPipes.pipes);
            free(outPipes.pipes);

            // Everything is ready for the slaves processes
            char *argvC[] = {"./slave", NULL};
            char *envpC[] = {NULL};
            if(execve("slave", argvC, envpC) == ERROR){
                perror("execve");
                exit(1);
            }
        } else{
            // Now we have to distribute evenly all the initial paths that the slaves have to process
            for(int i=0; i<MAX_PER_SLAVE && argCount < argc ; i++)
                for(int j=0; j<slavesAmount && argCount < argc; j++, argCount++)
                    write(inPipes.pipes[j][0], argv[argCount], strlen(argv[argCount]));
        }
    }

    // Now we can create a subprocess of the main program that can read all the data given by the slaves
    pid_t pid;
    if((pid = fork()) == ERROR){
        perror("fork");
        exit(1);
    }
    if(pid == 0){
        // We don't need the writing pipes if we are in this process
        for(int i=0; i<slavesAmount ;i++){
            if(close(inPipes.pipes[i][0]) == ERROR){
                perror("close");
                exit(1);
            }
            free(inPipes.pipes[i]);
        }
        free(inPipes.pipes);

        // Now we need to make all the synchronization process for reading all the outputs from the slaves

    } else{
        // If we are the mainProcess, we still need to keep writing the remaining paths evenly on the slaves' pipes
        // But first, we need to get rid of the pipes that we don't need
        for(int i=0; i<slavesAmount ;i++){
            if(close(outPipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(1);
            }
            free(inPipes.pipes[i]);
        }
        free(outPipes.pipes);
    }
    if(argCount < argc){
        // 
    }




    // Waits until ALL child processes finish their tasks
    wait(&status);
    return 0;
}