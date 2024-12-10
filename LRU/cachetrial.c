#include "headers.h"
#include "cachetrial.h"


int main() {
    init_cache();
    insert_at_front(1);
    insert_at_front(2);
    insert_at_front(3);
    insert_at_front(4);
    insert_at_front(5);
    print_cache();
    // move_to_front(3);
    // print_cache();
    // move_to_front(5);
    // print_cache();
    // move_to_front(1);
    // print_cache();
    // move_to_front(4);
    // print_cache();
    // move_to_front(2);
    // print_cache();
    return 0;
}

int cache_size = 0;

node* cache_array[MAX_CACHE_SIZE];

node* head = NULL;

void init_cache() {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        cache_array[i] = NULL;
    }
}

void print_cache() {
    for (int i = 0; i < MAX_CACHE_SIZE; i++) {
        if (cache_array[i] == NULL) {
            printf("NULL ");
        } else {
            node *cur = cache_array[i];
            while (cur != NULL) {
                printf("%d ", cur->data);
                cur = cur->next_in_hash;
            }
        }
    }
    printf("\n");
}

int hash(int data) {
    return data % MAX_CACHE_SIZE;
}

bool is_in_cache(int data) {
    int index = hash(data);
    node *cur = cache_array[index];
    while (cur != NULL) {
        if (cur->data == data) {
            return true;
        }
        cur = cur->next_in_hash;
    }
    return false;
}

void remove_last_and_insert(int data){
    int index = hash(data);
    node* cur = cache_array[index];
    while(cur->next_in_hash != NULL){
        cur = cur->next_in_hash;
    }
    cur->prev->next = NULL;
    cur->prev = NULL;
    cur->next = cache_array[index];
    cache_array[index]->prev = cur;
    cache_array[index] = cur;

}

void insert_at_front(int data){
    int index = hash(data);
    if(head == NULL){
        head = (node*)malloc(sizeof(node));
        head->data = data;
        head->next = NULL;
        head->prev = NULL;
        head->next_in_hash = NULL;
        cache_array[index] = head;
    }
    else{
        node* new_node = (node*)malloc(sizeof(node));
        new_node->data = data;
        new_node->next = head;
        new_node->prev = NULL;
        new_node->next_in_hash = NULL;
        head->prev = new_node;
        node* cur = cache_array[index];
        while(cur->next_in_hash != NULL){
            cur = cur->next_in_hash;
        }
        cur->next_in_hash = new_node;
    }
}

bool move_to_front(int data){
    int index = hash(data);
    node *cur = cache_array[index];
    if(cur->data == data){
        node* previous_node = cur->prev;
        node* next_node = cur->next;
        return true;
    }
    while (cur != NULL) {
        if (cur->next_in_hash->data == data) {
            if(cur->prev != NULL){
                cur->prev->next = cur->next;
            }
            if(cur->next != NULL){
                cur->next->prev = cur->prev;
            }
            cur->next = cache_array[index];
            cur->prev = NULL;
            cache_array[index]->prev = cur;
            cache_array[index] = cur;
            return true;
        }
        cur = cur->next_in_hash;
    }

    return false;





}
