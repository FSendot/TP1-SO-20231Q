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

ssize_t getline(char **lineptr, size_t *n, FILE *stream);

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
        size_t pathlen = strlen(path);
        char *buffer = malloc((HASH_LENGTH + pathlen + 3)*sizeof(char));
        read(pipefd[0], buffer, HASH_LENGTH + pathlen + 3);
        buffer[HASH_LENGTH+pathlen+2] = '\0';
        printf("Process ID: %d | Hash Value and Filename: %s\n", getpid(), buffer);
        
        free(buffer);
        close(pipefd[0]);
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
    char *linePointer = NULL;
    size_t lineCap = 0;
    ssize_t lineLen;

    // The linePointer will contain allocated memory on the programm, that has to be freed after using it
    while((lineLen = getline(&linePointer, &lineCap, stdin)) > 0){
        linePointer[lineLen - 1] = '\0';

        launchMD5(linePointer);

        free(linePointer);
        linePointer = NULL;
        lineCap = 0;
    }

    return 0;
}