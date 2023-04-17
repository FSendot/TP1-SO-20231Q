#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

#include "shared_memory.h"

#define STDIN 0
#define BUFFER_SIZE 512


ssize_t getline(char **lineptr, size_t *n, FILE *stream);

int main(int argc, char *argv[]) {
    int shared_mem_size = 0;  
    
    if (argc > 1) {
        shared_mem_size = atoi(argv[1]);
    } else {
        char *shared_mem_size_str = NULL;
        size_t n = 0;
        
        ssize_t line_len = getline(&shared_mem_size_str, &n, stdin);

        if (shared_mem_size_str[line_len-1] == '\n') {
                shared_mem_size_str[line_len-1] = '\0';
                shared_mem_size = atoi(shared_mem_size_str);    
        } else { 
            shared_mem_size = atoi(shared_mem_size_str);
        }

        free(shared_mem_size_str);
    }
    
    shared_memory_ADT shared_mem = open_shared_memory(shared_mem_size);
    char buffer[BUFFER_SIZE] = {0};

    int bytes_read = read_shared_memory(shared_mem, buffer, BUFFER_SIZE);
    while(bytes_read != EOF) {
        printf("%s", buffer);
        for(int i = 0; buffer[i] != '\0' ;i++){
            buffer[i] = '\0';
        }
        bytes_read = read_shared_memory(shared_mem, buffer, BUFFER_SIZE);
    }

    close_shared_memory(shared_mem);
    
    return EXIT_SUCCESS;
}