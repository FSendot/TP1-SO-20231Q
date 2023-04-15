
#define _GNU_SOURCE
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

#define ERROR -1

#define SHM_NAME "/SO-SHM"

// Se usa al escribir sobre la memoria compartida.
#define SHM_WRITE_SEM "/SO-WRITE-SEM"   



typedef struct shared_memory_CDT{
    void *shm_ptr;
    sem_t *mutex_ptr;
    sem_t *hashes_unread;
    size_t shm_size;
} shared_memory_CDT;

shared_memory_ADT initialize_shared_memory(size_t shm_size) {
    /* Access to shared memory writing must be done by one slave at a time, so initial
        value of semaphore has be 1.
       Access to read the hashes stored in shared memory needs a semaphore because the reader
       has to know if there are new hashes or block.
    */
    shared_memory_ADT shm = malloc(sizeof(shared_memory_CDT));

    shm->shm_size = shm_size;
    shm->mutex_ptr=sem_open(SHM_WRITE_SEM, O_RDWR | O_CREAT, 0666, 0);
    shm->hashes_unread=sem_open(SHM__READ_SEM, O_RDWR | O_CREAT, 0666, 0);

    if(sem_post(shm->mutex_ptr) == ERROR){
        perror("sem_post");
        exit(EXIT_FAILURE);
    }


    //A new shared memory object initially has zero length--the size of the object.
    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
    if (shm_fd == ERROR) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }
    
    //Cause the regular file named by path or referenced by fd to be truncated to a size of precisely length bytes.
    if (ftruncate(shm_fd, shm_size) == ERROR) {
        perror("ftruncate");
        exit(EXIT_FAILURE);
    }

    //FIXME 
    shm->shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, shm_fd, 0);
    if (shm->shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }

    *(char*)(shm->shm_ptr) = INITIAL_TOKEN;

    return shm;
}

void* open_shared_memory(size_t shm_size) {
    int shm_fd = shm_open(SHM_NAME, O_RDWR, 0666);
    if (shm_fd == ERROR) {
        perror("shm_open");
        exit(EXIT_FAILURE);
    }

    void* shm_ptr = mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shm_ptr == MAP_FAILED) {
        perror("mmap");
        exit(EXIT_FAILURE);
    }
    

    return shm_ptr;
}

//FIXME, eliminar parametro size
void write_to_shared_memory(shared_memory_ADT shm, char* buffer, size_t size) {
    /*
        //FIXME
        Se podría mejorar, teniendo un contador de cuantos hashes ya se guardaron, e ir directo
        a escribir a la posición proxima a escribir.
    */

    printf("hola\n");
    if(sem_wait(shm->mutex_ptr) == ERROR){
        perror("sem_wait");
        exit(EXIT_FAILURE);
    }
    //FIXME
    void *shm_ptr = shm->shm_ptr;

    if (*((char*)shm_ptr) == INITIAL_TOKEN) {
        strcpy(shm_ptr, buffer);
    } else {
        char* lastAppearance = strrchr(shm_ptr, SPLIT_TOKEN);
        lastAppearance++; 
        memcpy(lastAppearance, buffer, size);
    }

    if(sem_post(shm->mutex_ptr) == ERROR){
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
    if(sem_post(shm->hashes_unread) == ERROR){
        perror("sem_post");
        exit(EXIT_FAILURE);
    }
}

void unlink_shared_memory_resources(shared_memory_ADT shm) {
    //FIXME: No se si está bien...
    sem_close(shm->hashes_unread);
    sem_close(shm->mutex_ptr);
    
    if(munmap(shm->shm_ptr, shm->shm_size)==ERROR){
        perror("munmap");
        exit(EXIT_FAILURE);
    } if(shm_unlink(SHM_NAME)==ERROR){
        perror("shm_unlink");
        exit(EXIT_FAILURE);
    }
    free(shm);
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