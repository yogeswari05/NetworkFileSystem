#ifndef _NS_H_
#define _NS_H_

#include "headers.h"

#define MAX_STORAGE_SERVERS 10
#define MAX_CLIENTS 50
#define BUFFER_SIZE 1024
#define MAX_PATH_LENGTH 256
#define HASH_TABLE_SIZE 100 // Size of the hash map
#define MAX_ACCESSIBLE_PATHS 100

typedef struct {
   char ip_address[16];
   int nm_port;
   int client_port;
   int server_port;
   char accessible_paths[MAX_ACCESSIBLE_PATHS][MAX_PATH_LENGTH];
   int num_paths;
   int socket;
   int is_active;
} StorageServer;

typedef struct HashNode {
    char path[MAX_PATH_LENGTH];        // Key: Path
    StorageServer *server;             // Value: Pointer to StorageServer
    struct HashNode *next;             // Linked list for collision handling
} HashNode;

typedef struct {
    HashNode *table[HASH_TABLE_SIZE];  // Hash table array
    pthread_mutex_t lock;              // Mutex for thread safety
} HashMap;


typedef struct {
    StorageServer storage_servers[MAX_STORAGE_SERVERS];
    int num_storage_servers;
    HashMap path_to_server_map;        // Hash map for path-to-server mapping
    pthread_mutex_t lock;
} NamingServer;

extern NamingServer naming_server;

unsigned int        hash(const char *key);
void                hash_map_init(HashMap *map);
void                hash_map_insert(HashMap *map, const char *path, StorageServer *server);
StorageServer*      hash_map_find(HashMap *map, const char *path);
void                hash_map_print(HashMap *map);
void                handle_storage_server_registration(int client_socket);
void                handle_client_request(int client_socket);
void                handle_naming_server(void *nm_socket);
void*               connection_handler(void *socket_desc);

#endif