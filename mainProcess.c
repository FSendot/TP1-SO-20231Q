// This is a personal academic project. Dear PVS-Studio, please check it.
// PVS-Studio Static Code Analyzer for C, C++ and C#: http://www.viva64.com
#define _GNU_SOURCE
#include <stdio.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <sys/stat.h>        // For mode constants
#include <fcntl.h>           // For O_* constants
#include "shared_memory.h"

#define STDIN 0
#define STDOUT 1
#define ERROR -1

#define PIPE_BUFF 512
#define SLAVE_HASH_OUTPUT 106   // Counts it as a string.

// Gcc doesn't find the propotype of this function, we don't know why, so we write it in here.

void setlinebuf(FILE *stream);

// Data structure useful for creating a group of pipes used by the main process.
typedef struct pipe_group{
    int **pipes;
} pipe_group;


int main(int argc, char *argv[]) {
    // argsv[] = {"./mainProcess", "./archivo1", "./archivo2"} 
    // argc = mainPath + #arguments

    // Pipes used for writing on the worker processes.

    pipe_group in_pipes;

    
    // Pipes used for read the worker processes results.

    pipe_group out_pipes;

    
    int status;
    char aux_buffer[PIPE_BUFF] = {0};
    
    // The program path/name doesn't count, so we start to counting one after.

    int arg_count = 1;
    
    /* 
    **  Slaves are created in a log2 order of input, it reduces drastically 
    **  the amount of processes created for each group of files.
    */ 
    
    int slaves_amount = (int) log2((double) argc - 1);
    int initial_paths = slaves_amount / 2;


    /* 
    **  If we receive only 1 argument -> log2(1) = 0, so we need to increment slaves_amount 
    **  for the program to have at least 1 slave.
    */
    
    if(slaves_amount < 1){
        slaves_amount++;
        initial_paths++;
    }

    in_pipes.pipes = malloc(slaves_amount*sizeof(int *));
    out_pipes.pipes = malloc(slaves_amount*sizeof(int *));

    if(in_pipes.pipes == NULL || out_pipes.pipes == NULL){
        perror("malloc");
        exit(EXIT_FAILURE);
    }


    // We now create all the pipes that we're going to need.

    for(int i = 0; i < slaves_amount ;i++){
        in_pipes.pipes[i] = malloc(2*sizeof(int));
        out_pipes.pipes[i] = malloc(2*sizeof(int));

        if(in_pipes.pipes[i] == NULL || out_pipes.pipes[i] == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }
        
        if(pipe(in_pipes.pipes[i]) == ERROR || pipe(out_pipes.pipes[i]) == ERROR){
            perror("pipe");
            exit(EXIT_FAILURE);
        }
    }

    pid_t pid;
    for (int i = 0; i < slaves_amount ;i++) {
        if((pid = fork()) == ERROR){
            perror("fork");
            exit(EXIT_FAILURE);
        }

        if(pid == 0){
            int aux;

            //close(STDIN)
            //dup(in_pipes.pipes[i][0])      
            
            if((aux = dup2(in_pipes.pipes[i][0], STDIN)) == ERROR || aux != STDIN){     // r-end of pipe
                perror("dup2");
                exit(EXIT_FAILURE);
            }

            if((aux = dup2(out_pipes.pipes[i][1], STDOUT)) == ERROR || aux != STDOUT){  // w-end of pipe
                perror("dup");
                exit(EXIT_FAILURE);
            }


            /*
            **  No need for the pipes variables to exist in the child process.
            **  We can't have memory leaks anywhere, so we're freeing everything that we are not using
            **  and closing every fd that we are not using.
            */
            
            for(int j = 0; j < slaves_amount ;j++){
                if(close(in_pipes.pipes[j][0]) == ERROR || close(in_pipes.pipes[j][1]) == ERROR 
                        || close(out_pipes.pipes[j][0]) == ERROR || close(out_pipes.pipes[j][1])){
                    perror("close");
                    exit(EXIT_FAILURE);
                }
                free(in_pipes.pipes[j]);
                free(out_pipes.pipes[j]);
            }
            free(in_pipes.pipes);
            free(out_pipes.pipes);


            //  Everything is ready for the slaves processes.
            
            char *const argvC[] = {"./slave", NULL};
            char *envpC[] = {NULL};
            execve("./slave", argvC, envpC);
            

            // If reached this line, is because it was an error.

            perror("execve");
            exit(EXIT_FAILURE);
        }
    }


    // We need to close the rest of the pipes in the parent code before forking of doing anything else.

    for(int i = 0; i < slaves_amount ;i++){
        if(close(in_pipes.pipes[i][0]) == ERROR || close(out_pipes.pipes[i][1]) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }
    }


    // Now we have to distribute evenly all the initial paths that the slaves have to process.

    for(int i = 0; i < initial_paths && arg_count < argc ;i++){
        for(int j = 0; j < slaves_amount && arg_count < argc;j++, arg_count++){
            int k = 0;
            while(k < PIPE_BUFF && argv[arg_count][k] != '\0'){
                aux_buffer[k] = argv[arg_count][k];
                k++;
            }
            if(k < PIPE_BUFF - 1){
                aux_buffer[k++] = '\n';
                aux_buffer[k] = '\0';
            } else{
                aux_buffer[k] = '\0';
            }
            
            write(in_pipes.pipes[j][1], aux_buffer, k);
        }
    }


    // Now we can create a subprocess of the main program that can read all the data given by the slaves.

    if((pid = fork()) == ERROR){
        perror("fork");
        exit(EXIT_FAILURE);
    }

    
    if(pid == 0){
        
        //  View will start showing the workers' results after receiving the output given by this process.

        size_t shared_memory_size = (argc-1)*SLAVE_HASH_OUTPUT + 1024;
        shared_memory_ADT shared_mem = initialize_shared_memory(shared_memory_size);
        printf("%d\n", (int) shared_memory_size);
        if(fflush(stdout) == EOF){
            perror("fflush");
            exit(EXIT_FAILURE);
        }

        int output_fd;
        if((output_fd = open("output.txt", O_WRONLY | O_CREAT)) == ERROR){
            perror("open");
            exit(EXIT_FAILURE);
        }
        

        // We don't need the writing pipes if we are in this process.

        for(int i = 0; i < slaves_amount ;i++){
            if(close(in_pipes.pipes[i][1]) == ERROR){
                perror("close");
                exit(EXIT_FAILURE);
            }
            free(in_pipes.pipes[i]);
        }
        free(in_pipes.pipes);

        /*
        **  Now we need to make all the synchronization process for reading all the outputs from the slaves
        **  is blocked waiting for the childs to be on ready state.
        */

        fd_set read_fd;
        int fd_amount, max_fd;

        max_fd = 0;

        /*
        **  We need to create stream variables for the pipes that we can use for reading all the results
        **  without any risk of reading more than we should.
        */

        FILE **pipe_streams;
        if((pipe_streams = malloc(slaves_amount*sizeof(FILE *))) == NULL){
            perror("malloc");
            exit(EXIT_FAILURE);
        }

        for(int i = 0; i < slaves_amount ;i++){
            max_fd = out_pipes.pipes[i][0] > max_fd ? out_pipes.pipes[i][0] : max_fd;
            if((pipe_streams[i] = fdopen(out_pipes.pipes[i][0], "r")) == NULL){
                perror("fdopen");
                exit(EXIT_FAILURE);
            }

            /* 
            **  The worker leaves a new line after every result, that is useful for us to 
            **  separate each result correctly.
            */

            setlinebuf(pipe_streams[i]);
        }

        max_fd++;


        char *buffer_ptr;
        int *closed_pipes = calloc(slaves_amount, sizeof(int));
        if(closed_pipes == NULL){
            perror("calloc");
            exit(EXIT_FAILURE);
        }

        int closed_pipes_amount = 0;

        
        // We need to process the rest of the programs arguments.

        while(closed_pipes_amount < slaves_amount){
            
            // Initialize the File Descriptor set for the use of select.
            
            FD_ZERO(&read_fd);
            for(int i = 0; i<slaves_amount ;i++){
                if(closed_pipes[i] != 1)
                    FD_SET(out_pipes.pipes[i][0], &read_fd);
            }

            if((fd_amount = select(max_fd, &read_fd, NULL, NULL, NULL)) == ERROR){
                perror("select");
                exit(EXIT_FAILURE);
            }

            // For every every result that is ready, we can raed it and write it on the shared memory.
            
            for(int i = 0; i < slaves_amount && fd_amount > 0 ;i++){
                if(closed_pipes[i] != 1 && FD_ISSET(out_pipes.pipes[i][0], &read_fd)){
                    buffer_ptr = fgets(aux_buffer, PIPE_BUFF, pipe_streams[i]);
                
                    // EOF reached on that pipe

                    if(buffer_ptr == NULL){
                        if(close(out_pipes.pipes[i][0]) == ERROR){
                            perror("close");
                            exit(EXIT_FAILURE);
                        }
                        free(out_pipes.pipes[i]);
                        closed_pipes[i] = 1;
                        closed_pipes_amount++;
                    } else{
                        int length = strlen(aux_buffer);
                        write_to_shared_memory(shared_mem,aux_buffer, length);
                        write(output_fd, aux_buffer,length);
                    }

                    fd_amount--;
                }
            }
        }

        // Every pipe is closed, now we need to free the remaining memory spaces and finish this process
        if(fcloseall() == EOF){
            perror("fcloseall");
            exit(EXIT_FAILURE);
        }
        free(out_pipes.pipes);
        free(closed_pipes);
        free(pipe_streams);

        // We notify to the shared memory that we finished writing in it

        strcpy(aux_buffer, END_STR);
        write_to_shared_memory(shared_mem, aux_buffer, strlen(aux_buffer));

        sleep(4);
  
        unlink_shared_memory_resources(shared_mem);
        if(close(output_fd) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }

        return EXIT_SUCCESS;
    }

    /* 
    **  If we are the father of all processes, we still need to keep writing the remaining paths evenly
    **  on the slaves' pipes. But first, we need to get rid of the pipes that we don't need.
    */

    for(int i = 0; i < slaves_amount ;i++){
        if(close(out_pipes.pipes[i][0]) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }
        free(out_pipes.pipes[i]);
    }
    free(out_pipes.pipes);

    fd_set write_fd;
    int fd_amount, max_fd;

    max_fd = 0;
    
    for(int i = 0; i < slaves_amount ;i++){
        max_fd = in_pipes.pipes[i][1] > max_fd ? in_pipes.pipes[i][1] : max_fd;
    }
    
    max_fd++;

    // We need to process the rest of the programs arguments.

    while(arg_count < argc){
        
        // Initialize the File Descriptor set for the use of select.
        FD_ZERO(&write_fd);
        
        for(int i = 0; i<slaves_amount ;i++){
            FD_SET(in_pipes.pipes[i][1], &write_fd);
        }

        if((fd_amount = select(max_fd, NULL, &write_fd, NULL, NULL)) == ERROR){
            perror("select");
            exit(EXIT_FAILURE);
        }

        // For every process that is ready, we can write on it the next file that needs to be processed.

        for(int i = 0; i < slaves_amount && arg_count < argc ;i++){
            if(FD_ISSET(in_pipes.pipes[i][1], &write_fd)){
                int k = 0;
                while(k < PIPE_BUFF && argv[arg_count][k] != '\0'){
                    aux_buffer[k] = argv[arg_count][k];
                    k++;
                }
                if(k < PIPE_BUFF - 1){
                    aux_buffer[k++] = '\n';
                    aux_buffer[k] = '\0';
                } else{
                    aux_buffer[k] = '\0';
                }
                write(in_pipes.pipes[i][1], aux_buffer, k);

                arg_count++;
            }
        }
    }

    // No arguments left, now we need to close the pipes and free the heap

    for(int i = 0; i < slaves_amount ;i++){
        if(close(in_pipes.pipes[i][1]) == ERROR){
            perror("close");
            exit(EXIT_FAILURE);
        }
        free(in_pipes.pipes[i]);
    }
    free(in_pipes.pipes);

    // Waits until ALL child processes finishes their tasks

    while(waitpid(-1, &status, 0) > 0);
    
    if(status != 0){
        perror("Status of some child");
        exit(EXIT_FAILURE);
    }
    return EXIT_SUCCESS;
}
