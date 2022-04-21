#include <stdbool.h>

#ifndef LOCK_FREE_IMPL_H
#define LOCK_FREE_IMPL_H
typedef struct node node_t;
typedef struct list list_t;

struct node {
    node_t **next; // top_level-sized array of node pointers
    void *value;
    int key;
    int top_level;
    int refcount; // used to prevent deletions
};

struct list {
    node_t *leftmost;
    double p;
};

list_t list_init(double p);

void *list_free(list_t *l);

bool list_insert(list_t *l, int key, void *value);

void *list_lookup(list_t *l, int key);

#endif