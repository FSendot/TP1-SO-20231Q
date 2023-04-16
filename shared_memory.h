#include <stdlib.h>


#define SPLIT_TOKEN '\n'
#define INITIAL_TOKEN '&'
#define END_TOKEN '$'
#define ERROR -1
// #define SHM_SIZE 1048576    // 1MB - Se calcula directo en el main process.

typedef struct shared_memory_CDT *shared_memory_ADT;

/*
    Se inicializa la shared memory, es decir, crea la estructura vacía.
*/
shared_memory_ADT initialize_shared_memory(size_t shm_size);

/*
    Abre la shared memory, ya debidamente inicializada, y la mapea(dandole el tamaño) al proceso actual.
*/
shared_memory_ADT open_shared_memory(size_t shm_size);
void write_to_shared_memory(shared_memory_ADT shm, char* buffer, size_t size);
int read_shared_memory(shared_memory_ADT shm, char *buffer);
void unlink_shared_memory_resources(shared_memory_ADT shm);
void close_shared_memory(shared_memory_ADT shm);



void show_shared_memory(shared_memory_ADT shm);