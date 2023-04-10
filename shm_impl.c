
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


size_t create_shared_memory(size_t hashes_count) {
    //For every hash, a token will be added.
    size_t shm_size = 16 * (hashes_count + 1);

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

    return shm_size;
}

void* open_shared_memory(size_t shm_size) {
    
    int shm_fd = shm_open(SHM_NAME, O_RDWR | O_CREAT, 0666);
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


void write_to_shared_memory(void* shm_ptr, char* buffer, size_t size) {

    /* Access to shared memory writing must be done by one slave at a time, so initial
        value of semaphore has be 1.
    */
    sem_t* sem = sem_open(SHM_SEM, O_CREAT, 0666, 1);
    
    /*
        //FIXME
        Se podría mejorar, teniendo un contador de cuantos hashes ya se guardaron, e ir directo
        a escribir a la posición proxima a escribir.
    */
    sem_wait(sem);  
    char* lastAppearance = strrchr(shm_ptr, SPLIT_TOKEN);
    if(lastAppearance == NULL){
        lastAppearance = (char*)shm_ptr;
    } else { lastAppearance++; }
    strcpy(lastAppearance, buffer);
    *(lastAppearance + size) = '&';
    sem_post(sem);


    sem_close(sem);
}

void unlink_shared_memory_resources() {
}