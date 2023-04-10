#include <stdlib.h>

#define SHM_NAME "/SO-SHM"
#define SHM_SEM "/SO-SEM"
#define SPLIT_TOKEN '&'
#define ERROR -1
// #define SHM_SIZE 1048576    // 1MB - Se calcula directo en el main process.

size_t create_shared_memory(size_t hashes_count);
void* open_shared_memory(size_t shm_size);
void write_to_shared_memory(void* shm_ptr, char* buffer, size_t size);
void unlink_shared_memory_resources(void*shm_ptr, size_t shm_size);