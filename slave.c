#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define HASH_LENGTH 32
#define PIPE_BUFF 512

void launchMD5(char *path){
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
        write(STDOUT, writeBuffer, PIPE_BUFF);
    } else{
        close(pipefd[0]);
        perror("md5sum");
        exit(1);
    }

    return;
}

int main(int argc, char *argv[]){
    //If slave receives arguments
    if(argc > 1){
        for(int i=1; i<argc ;i++){
            launchMD5(argv[i]);
        }
        return 0;
    }

    // Initialize variables for getline()
    char buffer[PIPE_BUFF] = {0};
    ssize_t lineLen;

    // The linePointer will contain allocated memory on the programm, that has to be freed after using it
    while((lineLen = read(STDIN, buffer, PIPE_BUFF)) > 0){
        if(buffer[strlen(buffer)-1] == '\n' ) buffer[strlen(buffer)-1] = '\0';
        launchMD5(buffer);
        for(int i=0; buffer[i] != '\0' ;i++) buffer[i] = '\0';
    }

    return 0;
}