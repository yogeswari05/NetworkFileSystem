#ifndef CACHETRIAL_H
#define CACHETRIAL_H

#include "headers.h"

void init_cache();
void print_cache();
int hash(int data);
bool is_in_cache(int data);
void remove_last_and_insert(int data);
void insert_at_front(int data);
bool move_to_front(int data);

#define MAX_CACHE_SIZE 5

typedef struct node {
    int data;
    struct node *next;
    struct node *prev;
    struct node *next_in_hash;

    // struct node *tail;
} node;

#endif