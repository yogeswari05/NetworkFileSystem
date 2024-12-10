#ifndef _CACHE2_H_
#define _CACHE2_H_

#include "headers.h"
#include "namingServer.h"   

typedef struct node {
    char string[MAX_STRING_SIZE];
    StorageServer* server;
    struct node* next;
    struct node* prev;
    struct node* next_in_hash;
} node;

void init_cache();
void print_cache();
node* find_in_hash(const char* string);
void remove_from_hash(node* target);
void move_to_front(node* target);
void insert_at_front(const char* string);
StorageServer* access_cache(const char* string);
void add_to_hash(node* new_node);
unsigned int hashstring(const char* key);
void remove_last();
bool is_in_cache(const char* key);

extern int cache_size;
extern node* cache_array[HASH_TABLE_SIZE];
extern node* head;
extern node* tail;

#endif