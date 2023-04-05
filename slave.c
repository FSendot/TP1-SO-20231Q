#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define HASH_LENGTH 16

int main(int argc, char *argv[]){
    //If slave receives arguments
    if(argc > 1){
    }

    // Initialize variables for getline()
    char *linePointer = NULL;
    size_t lineCap = 0;
    int linelen;

    // The linePointer will contain allocated memory on the programm, that has to be freed after using it
    while((linelen = getline(&linePointer, &lineCap, stdin)) > 0){
        // Pipe that will ne used for communicate with md5sum
        int pipefd[2];
        if(pipe(pipefd) == ERROR){
            perror("pipe");
            exit(1);
        }
        pid_t pid;
        int status;

        if((pid = fork()) == ERROR){
            perror("fork");
            exit(1);
        }
        if(pid == 0){
            dup2(pipefd[1], STDOUT);
            close(pipefd[1]);
            close(pipefd[0]);

            char *argvC[] = {"/usr/bin/md5sum", linePointer, NULL};
            char *envpC[] = {NULL};
            printf("\n%s\n", linePointer);
            if(execve("/usr/bin/md5sum", argvC, envpC) == ERROR){
                perror("execve");
                return 1;
            }
        }
        close(pipefd[1]);
        wait(&status);
        if(status == 0){
            char buffer[HASH_LENGTH + 1];
            read(pipefd[0], buffer, HASH_LENGTH);
            buffer[HASH_LENGTH] = '\0';
            printf("Process ID: %d | Hash Value: %s | Filename: %s", getpid(), buffer, linePointer);
            
            free(linePointer);
            lineCap = 0;
            
            close(pipefd[0]);
        } else{
            perror("md5sum");
            return 1;
        }
    }

    return 0;
}