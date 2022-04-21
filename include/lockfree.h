#include "skiplist.h"
#include "lockfree_c.h"
#ifndef LOCK_FREE_H
#define LOCK_FREE_H

template <typename T>
class LockFreeList : public SkipList<T> {
    private:
    list_t *_list;

    public:
    LockFreeList(int max_level, double p) : SkipList<T>(max_level, p) {
        _list = list_init(p);
    }
    ~LockFreeList() {
        list_free(_list);
    }
    bool insert(int key, T *value) { return list_insert(_list, key, static_cast<void *>(value)); }
    T *remove(int key) { return static_cast<T *>(list_remove(key)); }
    T *lookup(int key) { return static_cast<T *>(list_lookup(key)); }
};
#endif