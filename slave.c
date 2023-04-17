#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define PIPE_BUFF 512

// Gcc doesn't find the propotype of these functions, we don't know why, so we write them in here.

void setlinebuf(FILE *stream);
ssize_t getline(char **linePtr, size_t *n, FILE *stream);

void launch_md5(char *path){
    // Pipe that will ne used for communicate with md5sum
    int pipe_fd[2];

    if(pipe(pipe_fd) == ERROR){
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
        dup2(pipe_fd[1], STDOUT);
        close(pipe_fd[1]);
        close(pipe_fd[0]);

        char *argvC[] = {"/usr/bin/md5sum", path, NULL};
        char *envpC[] = {NULL};
        
        if(execve("/usr/bin/md5sum", argvC, envpC) == ERROR){
            perror("execve");
            exit(1);
        }
    }
    close(pipe_fd[1]);
    wait(&status);
    
    
    if(status == 0){
        char read_buffer[PIPE_BUFF] = {0};

        read(pipe_fd[0], read_buffer, PIPE_BUFF);
        close(pipe_fd[0]);

        printf("Process ID: %d | Hash Value and Filename: %s", getpid(), read_buffer);
        if(fflush(stdout) == EOF){
            perror("fflush");
            exit(EXIT_FAILURE);
        }
    } else{
        close(pipe_fd[0]);
        perror("md5sum");
        exit(EXIT_FAILURE);
    }
}

int main(int argc, char *argv[]){
    //  If slave receives arguments
    if(argc > 1){
        for(int i=1; i < argc ;i++){
            launch_md5(argv[i]);
        }
        return EXIT_SUCCESS;
    }

    // Initialize variables for getline()
    char *buffer = NULL;
    size_t n = 0;
    ssize_t line_len = 0;
    errno = 0;

    // The linePointer will contain allocated memory on the programm, that has to be freed after using it
    while((line_len = getline(&buffer, &n, stdin)) > 0){
        // We need to get rid of the '\n' character for md5sum
        buffer[line_len-1] = buffer[line_len-1] == '\n' ? '\0': buffer[line_len-1];
        launch_md5(buffer);
        free(buffer);
        buffer = NULL;
        n = 0;
    }

    // Last check if getline failed for any reason and/or couldn't free the memory alloc of the buffer
    if(errno != 0){
        if(buffer != NULL){
            free(buffer);
        }
        perror("getline");
        exit(EXIT_FAILURE);
    }

    return EXIT_SUCCESS;
}