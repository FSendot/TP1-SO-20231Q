#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <time.h>

//Shared memory
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include "shared_memory.h"


#define STDIN 0
#define BUFFER_SIZE 512




int main(int argc, char *argv[]) {
    int shm_size = 0;
    
    if (argc > 1) {
        shm_size = atoi(argv[1]);
    } else {
        char* shm_size_str = malloc(sizeof(char) * BUFFER_SIZE);

        while(read(STDIN, shm_size_str, BUFFER_SIZE) > 0);
        int str_len = strlen(shm_size_str);
        if (shm_size_str[str_len-1] == '\n') {
                shm_size_str[str_len-1] = '\0';   //Because, from STDIN, the number comes styled like 512\n\0
                shm_size = atoi(shm_size_str);    
        }
        shm_size = atoi(shm_size_str);

        free(shm_size_str);
    }

    sem_t* sem_reader = sem_open(SHM__READ_SEM, O_CREAT, 1);
    char* shm_ptr = (char*) open_shared_memory(shm_size);
    
    while(1) {
        sem_wait(sem_reader);
        
        char* read_ptr = (char*)shm_ptr;
        while (*read_ptr != SPLIT_TOKEN) {
            putchar(*read_ptr);
            read_ptr++;
        }
        putchar('\n');
    }
    

    return 0;
}
    // Hasta acá ya está chequeado que funciona.
/*
    sem_t* sem_reader = sem_open(SHM__READ_SEM, 0);

    char* shm_ptr = (char*) open_shared_memory(shm_size);

    int i = 0;
    while (1) {
        printf("%d. Vemos si hay mas data\n", i);
        sem_wait(sem_reader);
        printf("Hay\n");
        i++;
        printf("%s\n", shm_ptr);
        shm_ptr = shm_ptr + strlen(shm_ptr) + 1;
        // sem_post(sem_reader); No se debe realizar acá. Lo hace el shm_impl cuando escribe un nuevo hash.
    }

    return 0;   //No debería llegar.
}
*/