#include <stdlib.h>

#define SHM_NAME "/SO-SHM"

// Se usa al escribir sobre la memoria compartida.
#define SHM_WRITE_SEM "/SO-WRITE-SEM"   

// Se usa para saber cuantos hashes sin leer hay en la memoria compartida. : main_process <-> vista
#define SHM__READ_SEM "/SO-READ-SEM"


#define SPLIT_TOKEN '&'
#define ERROR -1
// #define SHM_SIZE 1048576    // 1MB - Se calcula directo en el main process.

/*
    Se inicializa la shared memory, es decir, crea la estructura vacía.
*/
int initialize_shared_memory(size_t shm_size);

/*
    Abre la shared memory, ya debidamente inicializada, y la mapea(dandole el tamaño) al proceso actual.
*/
void* open_shared_memory(size_t shm_size);

void write_to_shared_memory(void* shm_ptr, char* buffer, size_t size);
void unlink_shared_memory_resources(void*shm_ptr, size_t shm_size);