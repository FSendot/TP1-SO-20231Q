
#include <string.h>
#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>


//Shared memory
#include <sys/mman.h>
#include <sys/stat.h>        /* For mode constants */
#include <fcntl.h>           /* For O_* constants */
#include <semaphore.h>
#include "shared_memory.h"


int initialize_shared_memory(size_t shm_size) {
    /* Access to shared memory writing must be done by one slave at a time, so initial
        value of semaphore has be 1.
       Access to read the hashes stored in shared memory needs a semaphore because the reader
       has to know if there are new hashes or block.
    */
    sem_open(SHM_WRITE_SEM, O_RDWR | O_CREAT, 0666, 1);
    sem_open(SHM__READ_SEM, O_RDWR | O_CREAT, 0666, 0);


    //A new shared memory object initially has zero length--the size of the object.
    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }
    
    //Cause the regular file named by path or referenced by fd to be truncated to a size of precisely length bytes.
    if (ftruncate(shm_fd, shm_size) == -1) {
        perror("ftruncate");
        exit(1);
    }

    //FIXME 
    void* shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }

    *((char*) shm_ptr) = INITIAL_TOKEN;
    for (int i = 1; i < shm_size; i++) *(char*)shm_ptr = '\0';

    return shm_fd;
}

void* open_shared_memory(size_t shm_size) {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == -1) {
        perror("shm_open");
        exit(1);
    }

    void* shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(1);
    }
    

    return shm_ptr;
}

//FIXME, eliminar parametro size
void write_to_shared_memory(void* shm_ptr, char* buffer, size_t size) {
    sem_t* sem = sem_open(SHM_WRITE_SEM, O_RDWR | O_CREAT, 0666, 1);
    sem_t* hashesUnread = sem_open(SHM__READ_SEM, O_RDWR | O_CREAT, 0666, 0);

    /*
        //FIXME
        Se podría mejorar, teniendo un contador de cuantos hashes ya se guardaron, e ir directo
        a escribir a la posición proxima a escribir.
    */

    printf("wait(sem)\n");
    sem_wait(sem);  
    //FIXME
    if (*((char*)shm_ptr) == INITIAL_TOKEN) {
        strcpy(shm_ptr, buffer);
    } else {
        char* lastAppearance = strrchr(shm_ptr, SPLIT_TOKEN);
        lastAppearance++; 
        memcpy(lastAppearance, buffer, size);
    }

    sem_post(sem);
    printf("post(sem)\n");
    sem_post(hashesUnread);
    printf("post(hashesunread)\n");
}

void unlink_shared_memory_resources(void*shm_ptr, size_t shm_size) {
    //FIXME: No se si está bien...
    sem_t* sem = sem_open(SHM_WRITE_SEM, O_CREAT, 0666, 1);
    sem_close(sem);
    
    if(munmap(shm_ptr, shm_size)==ERROR){
        perror("munmap");
        exit(1);
    } if(shm_unlink(SHM_NAME)==ERROR){
        perror("shm_unlink");
        exit(1);
    }
}

/*
    Función super auxiliar.
*/
void show_shared_memory(void* shm_ptr, size_t shm_size) {
    printf("Comenzando a imprimir la shared memory.\n");
    int i = 0;
    while (i < shm_size) {
        printf("%c", (char) *((char*)shm_ptr + i));
        i++;
    }
    printf("Finalizado.\n");
}