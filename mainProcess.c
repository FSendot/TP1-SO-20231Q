#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define MAX_PER_SLAVE 3
#define INITIAL_SLAVES 5

// This variables are for handling any problem that the childs could give at processing the files
static volatile sig_atomic_t got_SIGCHLD = 0;

static void child_sig_handler(int sig){
    got_SIGCHLD = 1;
}

// Data structure useful for creating a group of pipes used by the main process
typedef struct pipeGroup{
    int **pipes;
} pipeGroup;

int main(int argc, char argv[]) {
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
    
    /* 
    ** If we wanted to add complexity to the amount of slaves
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
            exit(EXIT_FAILURE);
        }
        if(pipe(outPipes.pipes[i]) == ERROR){
            perror("pipe");
            exit(EXIT_FAILURE);
        }

        pid_t pid;
        if((pid = fork()) == ERROR){
            perror("fork");
            exit(EXIT_FAILURE);
        }
        if(pid == 0){
            if(dup2(inPipes.pipes[i][0], STDIN) == ERROR){
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if(close(inPipes.pipes[i][0]) == ERROR ||close(inPipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(EXIT_FAILURE);
            }

            if(dup2(outPipes.pipes[i][1], STDOUT) == ERROR){
                perror("dup2");
                exit(EXIT_FAILURE);
            }
            if(close(outPipes.pipes[i][1]) == ERROR || close(outPipes.pipes[i][0]) == ERROR){
                perror("close");
                exit(EXIT_FAILURE);
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
                exit(EXIT_FAILURE);
            }
        } else{
            // Now we have to distribute evenly all the initial paths that the slaves have to process
            for(int i=0; i<MAX_PER_SLAVE && argCount < argc ; i++)
                for(int j=0; j<slavesAmount && argCount < argc; j++, argCount++)
                    write(inPipes.pipes[j][1], argv[argCount], strlen(argv[argCount]));
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
            if(close(inPipes.pipes[i][0]) == ERROR || close(inPipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(EXIT_FAILURE);
            }
            free(inPipes.pipes[i]);
        }
        free(inPipes.pipes);

        // Now we need to make all the synchronization process for reading all the outputs from the slaves

    } else{
        // If we are the mainProcess, we still need to keep writing the remaining paths evenly on the slaves' pipes
        // But first, we need to get rid of the pipes that we don't need
        for(int i=0; i<slavesAmount ;i++){
            if(close(outPipes.pipes[i][0]) == ERROR || close(outPipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(EXIT_FAILURE);
            }
            free(outPipes.pipes[i]);
        }
        free(outPipes.pipes);

        // To avoid race conditions, we need to block signals thrown by the childs while the main process
        // is blocked waiting for the childs to be on ready state
        sigset_t empty_mask;
        fd_set writefd;
        int fdAmount, ndfs;

        sigemptyset(&empty_mask);

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
            if((fdAmount = pselect(ndfs, NULL, &writefd, NULL, NULL, &empty_mask)) == ERROR){
                perror("pselect");
                exit(EXIT_FAILURE);
            }
            // No slave processes free
            if(fdAmount == 0) continue;
            // For every process that is ready, we can write on it the next file that needs to be processed
            for(int i=0; i < slavesAmount && argCount < argc;i++){
                if(FD_ISSET(inPipes.pipes[i][1], &writefd)){
                    write(inPipes.pipes[i][1], argv[argCount], strlen(argv[argCount]));
                    argCount++;
                }
            }
        }
    }




    // Waits until ALL child processes finish their tasks
    wait(&status);
    return 0;
}