

#define SHM_NAME "/SO-SHM"
#define SHM_SEM "/SO-SEM"
#define SPLIT_TOKEN '&'
// #define SHM_SIZE 1048576    // 1MB - Se calcula directo en el main process.

void* open_shared_memory(size_t hashes_count, int prot);
void* write_to_shared_memory(void* shm_ptr, char* buffer, size_t size);
void unlink_shared_memory_resources();