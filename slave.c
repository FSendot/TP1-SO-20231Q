#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define PIPE_BUFF 512

ssize_t getline(char **linePtr, size_t *n, FILE *stream);

void launchMD5(char *path){
    // Pipe that will ne used for communicate with md5sum
    int pipefd[2];

    if(pipe(pipefd) == ERROR){
        perror("pipe");
        exit(EXIT_FAILURE);
    }
    pid_t pid;
    int status;

    if((pid = fork()) == ERROR){
            perror("fork");
            exit(EXIT_FAILURE);
        }
    if(pid == 0){
        dup2(pipefd[1], STDOUT);
        close(pipefd[1]);
        close(pipefd[0]);

        char *argvC[] = {"/usr/bin/md5sum", path, NULL};
        char *envpC[] = {NULL};
        
        if(execve("/usr/bin/md5sum", argvC, envpC) == ERROR){
            perror("execve");
            exit(1);
        }
    }
    close(pipefd[1]);
    wait(&status);
    
    
    if(status == 0){
        char readBuffer[PIPE_BUFF] = {0};

        read(pipefd[0], readBuffer, PIPE_BUFF);

        close(pipefd[0]);

        char writeBuffer[PIPE_BUFF] = {0};
        sprintf(writeBuffer, "Process ID: %d | Hash Value and Filename: %s", getpid(), readBuffer);
        writeBuffer[strlen(writeBuffer)-1] = '\0'; // Elimino el enter final que agrega md5sum al hash.
        write(STDOUT, writeBuffer, strlen(writeBuffer));
    
    } else{
        close(pipefd[0]);
        perror("md5sum");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]){
    //setvbuf(stdout, NULL, _IONBF, 0);

    //If slave receives arguments
    if(argc > 1){
        for(int i=1; i < argc ;i++){
            launchMD5(argv[i]);
        }
        return EXIT_SUCCESS;
    }

    // Initialize variables for getline()
    char *buffer = NULL;
    size_t n = 0;
    ssize_t lineLen = 0;

    // The linePointer will contain allocated memory on the programm, that has to be freed after using it
    while((lineLen = getline(&buffer, &n, stdin)) > 0){
        // We need to get rid of the '\n' character for md5sum
        buffer[lineLen-1] = buffer[lineLen-1] == '\n' ? '\0': buffer[lineLen-1];
        launchMD5(buffer);
        free(buffer);
        buffer = NULL;
        n = 0;
    }
    // Last check if getline failed for any reason and couldn't free the memory alloc of the buffer
    if(buffer != NULL)
        free(buffer);

    return EXIT_SUCCESS;
}