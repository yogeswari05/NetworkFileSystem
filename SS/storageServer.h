#ifndef _SS_H_
#define _SS_H_
#include "headers.h"

#define BUFFER_SIZE 4096
#define MAX_PATH_LENGTH 256
#define MAX_CLIENTS 20

typedef struct {
   int client_socket;
   int nm_socket;
} ClientHandler;

typedef struct {
   int nm_socket;
} NamingServerHandler;

struct write_args {
   char *path;       // Path to the file
   char *buffer;     // Data to be written
   size_t data_size; // Size of the data
   int nm_socket;    // Naming server socket
};

typedef struct _rwlock_t {
   sem_t lock;    // binary semaphore (basic lock)
   sem_t writelock; // used to allow ONE writer or MANY readers
   int readers; // count of readers reading in critical section
} rwlock_t;

extern rwlock_t rwlock;

void     rwlock_init(rwlock_t *rw);
void     rwlock_acquire_readlock(rwlock_t *rw);
void     rwlock_release_readlock(rwlock_t *rw);
void     rwlock_acquire_writelock(rwlock_t *rw);
void     rwlock_release_writelock(rwlock_t *rw);

void     get_accessible_paths(const char* base_path, char paths[][MAX_PATH_LENGTH], int* num_paths, int* base_path_added);
void     handle_delete(const char* path);
void     handle_read(int client_socket, const char* path);
void     handle_write(int client_socket, char* path, char* data, size_t data_size, int is_sync, int name_server_socket);
void     handle_get_file_info(int client_socket, char* path);
void     handle_stream_audio(int client_socket, char* path);
void*    handle_client(void* arg);
int      create_directories(const char *path);
int      is_directory(const char *path);
int      copy_file(const char *source, const char *destination);
int      copy_directory(const char *source, const char *destination);
void     send_file_or_directory(const char *path, int nm_socket);
int      create_directory(const char *path);
void     receive_file_or_directory(int nm_socket, char* basepath_dest);
void*    handle_naming_server(void* arg);
void*    async_write(void *arg);

#endif
