#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define MAX_CACHE_SIZE 5
#define HASH_TABLE_SIZE 100 // Size of the hash map
#define MAX_STRING_SIZE 50



typedef struct node {
    char string[MAX_STRING_SIZE];
    struct node* next;
    struct node* prev;
    struct node* next_in_hash;
} node;

int cache_size = 0;
node* cache_array[HASH_TABLE_SIZE];
node* head = NULL;
node* tail = NULL;

void init_cache() {
    for (int i = 0; i < HASH_TABLE_SIZE; i++){
        cache_array[i] = NULL;
    }
}

bool is_in_cache(const char* key) {
    unsigned int index = hashstring(key);
    node *cur = cache_array[index];
    while (cur != NULL) {
        if (strcmp(cur->string, key) == 0) {
            return true;
        }
        cur = cur->next_in_hash;
    }
    return false;
}

unsigned int hashstring(const char* key) {
    unsigned int hash = 0;
    while (*key) {
        hash = (hash * 31) + *key++;
    }
    return hash % HASH_TABLE_SIZE;
}

void print_cache() {
    node* cur = head;
    printf("Cache: ");
    while (cur) {
        printf("%s ", cur->string);
        cur = cur->next;
    }
    printf("\n");
}

node* find_in_hash(const char* string) {
    unsigned int index = hashstring(string);
    node* cur = cache_array[index];
    while (cur) {
        if (strcmp(cur->string, string) == 0) {
            return cur;
        }
        cur = cur->next_in_hash;
    }
    return NULL;
}

void remove_from_hash(node* target) {
    unsigned int index = hashstring(target->string);
    node* cur = cache_array[index];
    node* prev = NULL;

    while (cur) {
        if (cur == target) {
            if (prev) {
                prev->next_in_hash = cur->next_in_hash;
            } else {
                cache_array[index] = cur->next_in_hash;
            }
            return;
        }
        prev = cur;
        cur = cur->next_in_hash;
    }
}

void add_to_hash(node* new_node) {
    unsigned int index = hashstring(new_node->string);
    new_node->next_in_hash = cache_array[index];
    cache_array[index] = new_node;
}

void remove_last() {
    if (!tail) return;

    node* to_remove = tail;
    if (tail->prev) {
        tail->prev->next = NULL;
    } else {
        head = NULL;  // Cache is now empty
    }
    tail = tail->prev;

    remove_from_hash(to_remove);
    free(to_remove);
    cache_size--;
}

void move_to_front(node* target) {
    if (target == head) return;

    // Detach target from its current position
    if (target->prev) {
        target->prev->next = target->next;
    }
    if (target->next) {
        target->next->prev = target->prev;
    }

    // Update tail if needed
    if (target == tail) {
        tail = target->prev;
    }

    // Move to front
    target->next = head;
    target->prev = NULL;
    if (head) {
        head->prev = target;
    }
    head = target;

    // Update tail if list was previously empty
    if (!tail) {
        tail = head;
    }
}

void insert_at_front(const char* string) {
    node* new_node = (node*)malloc(sizeof(node));
    strncpy(new_node->string, string, MAX_STRING_SIZE);
    new_node->next = head;
    new_node->prev = NULL;
    new_node->next_in_hash = NULL;

    if (head) {
        head->prev = new_node;
    }
    head = new_node;

    if (!tail) {
        tail = head;
    }

    add_to_hash(new_node);
    cache_size++;
}

void access_cache(const char* string) {
    node* existing = find_in_hash(string);
    if (existing) {
        // Move to front if found
        printf("Cache hit: %s\n", string);
        move_to_front(existing);
    } else {
        printf("Cache miss: %s\n", string);
        // Evict if necessary
        if (cache_size == MAX_CACHE_SIZE) {
            remove_last();
        }
        // Insert new node
        insert_at_front(string);
    }
    print_cache();
}

int main() {
    init_cache();

    access_cache("hello");
    access_cache("world");
    access_cache("foo");
    access_cache("bar");
    access_cache("baz");
    access_cache("world");  // Move "world" to the front
    access_cache("qux");    // This will evict "hello"
    access_cache("foo");    // Move "foo" to the front

    return 0;
}
